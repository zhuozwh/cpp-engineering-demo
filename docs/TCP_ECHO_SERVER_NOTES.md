# TCP Echo Server 代码阅读笔记

本文档解释 v0.6 Tcp Echo Server 的代码结构和请求流程。面试回答维护在 `docs/PROJECT_INTERVIEW_GUIDE.md`。

---

## 1. 先理解两种 fd

TCP 服务端里至少有两类 fd。

### listen fd

listen fd 是服务器监听端口的入口。

```text
socket()
  ↓
bind()
  ↓
listen()
  ↓
得到 listen fd
```

listen fd 可读时，不是客户端发来了业务数据，而是内核已经完成了一个或多个 TCP 连接。此时要调用：

```cpp
accept(listen_fd, ...)
```

`accept()` 会返回新的连接 fd。

### 连接 fd

连接 fd 表示服务器与某个客户端之间已经建立好的 TCP 连接。

连接 fd 可读时，通常有两种情况：

- 客户端发来了字节数据，需要调用 `read()`。
- 客户端关闭了连接，`read()` 返回 `0`。

### 为什么要分开处理

```text
listen fd 可读
  ↓
accept()
  ↓
获得新的连接 fd

连接 fd 可读
  ↓
read()
  ↓
获得客户端发送的数据
```

因此代码里拆成：

- `Acceptor`：只处理 listen fd。
- `TcpConnection`：只处理一个连接 fd。

---

## 2. 一次 Echo 请求的完整流程

客户端发送 `"hello"` 后，完整链路如下：

```text
客户端 connect(127.0.0.1:8080)
  ↓
内核完成 TCP 握手
  ↓
listen fd 变为可读
  ↓
EventLoop -> Channel -> Acceptor::handle_read()
  ↓
accept() 返回连接 fd
  ↓
TcpServer::handle_new_connection()
  ↓
创建 TcpConnection
  ↓
TcpConnection::connect_established()
  ↓
连接 fd 注册 EPOLLIN
  ↓
客户端 send("hello")
  ↓
连接 fd 变为可读
  ↓
EventLoop -> Channel -> TcpConnection::handle_read()
  ↓
read() 读取字节
  ↓
input_buffer_.append(...)
  ↓
message_callback(connection, "hello")
  ↓
echo demo 调用 connection->send("hello")
  ↓
write() 写回客户端
```

---

## 3. Acceptor

相关文件：

- `include/net/acceptor.h`
- `src/net/acceptor.cpp`

`Acceptor` 只负责监听端口和接受新连接。

### 关键成员

- `Socket listen_socket_`
  - 保存 listen fd。
- `Channel accept_channel_`
  - 监听 listen fd 的读事件。
- `NewConnectionCallback new_connection_callback_`
  - accept 成功后，把新的连接 fd 交给 `TcpServer`。
- `bool listening_`
  - 是否已经进入监听状态。

### Acceptor(EventLoop* loop, uint16_t port)

流程：

```text
创建非阻塞 TCP socket
  ↓
设置 SO_REUSEADDR
  ↓
bind(0.0.0.0:port)
  ↓
为 listen fd 创建 Channel
  ↓
设置 Channel 的读回调为 handle_read()
```

### listen()

流程：

```text
listen_socket_.listen()
  ↓
accept_channel_.enable_reading()
  ↓
listen fd 注册到 epoll
```

### handle_read()

作用：listen fd 可读时循环调用 `accept()`。

为什么循环 accept：

- 一次 epoll 通知到来时，内核队列里可能已经有多个新连接。
- 循环 accept，直到非阻塞 `accept4()` 返回暂时没有连接。

每 accept 一个连接，就调用：

```cpp
new_connection_callback_(std::move(connection), peer_addr);
```

把连接交给 `TcpServer`。

---

## 4. TcpConnection

相关文件：

- `include/net/tcp_connection.h`
- `src/net/tcp_connection.cpp`

`TcpConnection` 表示一个已经建立好的客户端连接。

### 关键成员

- `Socket socket_`
  - 保存连接 fd，析构时自动关闭。
- `Channel channel_`
  - 监听连接 fd 的读、写、关闭和错误事件。
- `buffer::Buffer input_buffer_`
  - 暂存客户端发来的数据。
- `buffer::Buffer output_buffer_`
  - 暂存暂时没有一次写完的数据。
- `MessageCallback message_callback_`
  - 收到消息后交给上层处理。
- `CloseCallback close_callback_`
  - 连接关闭时通知 `TcpServer` 从连接表删除。
- `bool connected_`
  - 当前连接是否有效。

### connect_established()

作用：把新连接注册到 Reactor。

流程：

```text
connected_ = true
  ↓
设置 read/write/close/error 回调
  ↓
channel_.enable_reading()
  ↓
连接 fd 注册 EPOLLIN
```

### handle_read()

作用：处理连接 fd 可读事件。

流程：

```text
循环 read(fd, data, 4096)
  ↓
n > 0：append 到 input_buffer_
n == 0：客户端关闭连接，handle_close()
EINTR：重新 read
EAGAIN：当前数据已读完，退出循环
其它错误：handle_error()
  ↓
从 input_buffer_ 取出当前数据
  ↓
调用 message_callback_
```

为什么循环 read：

- socket 是非阻塞的。
- 一次事件到来时，内核接收缓冲区可能有多段数据。
- 循环读取直到 `EAGAIN`，表示本轮数据已经读完。

### send(const std::string& message)

作用：向客户端发送数据。

流程：

```text
如果 output_buffer_ 为空
  ↓
先直接 write()
  ↓
如果没有全部写完
  ↓
剩余数据 append 到 output_buffer_
  ↓
enable_writing()
```

为什么需要 `output_buffer_`：

- 非阻塞 `write()` 不保证一次写完。
- 如果内核发送缓冲区暂时满了，剩余数据必须保存下来。
- fd 再次可写时，epoll 会返回 `EPOLLOUT`，继续发送。

### handle_write()

作用：连接 fd 可写时，继续发送 `output_buffer_` 中的数据。

当数据全部写完后：

```cpp
channel_.disable_writing();
```

为什么要取消写事件：

- socket 大多数时间都可写。
- 如果没有待发送数据还持续监听 `EPOLLOUT`，EventLoop 会频繁被唤醒，造成空转。

### handle_close()

作用：

- 设置 `connected_ = false`。
- 取消 Channel 的所有事件。
- 调用关闭回调，让 `TcpServer` 从连接表删除这个连接。

### handle_error()

当前简化实现里，错误直接按关闭连接处理。

---

## 5. TcpServer

相关文件：

- `include/net/tcp_server.h`
- `src/net/tcp_server.cpp`

`TcpServer` 负责组合 `Acceptor` 和多个 `TcpConnection`。

### 关键成员

- `EventLoop* loop_`
  - 主事件循环。
- `Acceptor acceptor_`
  - 接受新连接。
- `MessageCallback message_callback_`
  - 新连接收到消息后执行的业务回调。
- `std::unordered_map<int, std::shared_ptr<TcpConnection>> connections_`
  - 活跃连接表，key 是连接 fd。

注意：`connections_` 是活跃连接管理，不是连接池。

### start()

作用：调用 `acceptor_.listen()` 开始监听端口。

### handle_new_connection(Socket socket, const sockaddr_in& peer_addr)

作用：为新连接创建 `TcpConnection`。

流程：

```text
Acceptor accept 成功
  ↓
创建 TcpConnection
  ↓
设置消息回调
  ↓
设置关闭回调
  ↓
保存到 connections_
  ↓
connect_established()
```

### remove_connection(int fd)

作用：连接关闭后从 `connections_` 删除对应对象。

删除后：

- `shared_ptr` 引用数归零。
- `TcpConnection` 析构。
- 内部 `Socket` 析构。
- 连接 fd 被关闭。

---

## 6. echo_server_demo

相关文件：

- `examples/echo_server_demo.cpp`

demo 做了三件事：

1. 创建 `EventLoop`。
2. 创建监听 `8080` 端口的 `TcpServer`。
3. 设置消息回调：收到什么就发送什么。

核心代码：

```cpp
server.set_message_callback(
    [](const net::TcpConnection::Ptr& connection, const std::string& message) {
        connection->send(message);
    });
```

运行：

```bash
./build/echo_server_demo
```

另开终端测试：

```bash
nc 127.0.0.1 8080
```

输入文本，服务端会原样返回。

---

## 7. 当前实现边界

当前版本是学习用的最小 TCP Echo Server。

已完成：

- listen socket 创建、bind、listen。
- listen fd 通过 Reactor 监听。
- 新连接 accept。
- 活跃连接表管理。
- 连接 fd 的非阻塞 read/write。
- 输入 Buffer 和输出 Buffer。
- Echo demo。

未完成：

- 没有应用层消息协议，当前把本轮读到的数据直接当作消息。
- 没有 HTTP 解析。
- 没有跨线程 wakeup。
- 没有多 EventLoop。
- 没有连接超时清理。
- 没有正式测试框架和压测工具。
- 没有数据库或 Redis 连接池。

---

## 8. 阅读顺序

建议按这个顺序阅读：

1. `examples/echo_server_demo.cpp`
2. `include/net/tcp_server.h`
3. `src/net/tcp_server.cpp`
4. `include/net/acceptor.h`
5. `src/net/acceptor.cpp`
6. `include/net/tcp_connection.h`
7. `src/net/tcp_connection.cpp`

读完后要能回答：

- listen fd 可读和连接 fd 可读分别代表什么？
- 为什么 `Acceptor` 要循环调用 accept？
- 为什么 `TcpConnection::handle_read()` 要循环 read？
- 为什么非阻塞写需要 `output_buffer_`？
- 为什么发送完成后要取消 `EPOLLOUT`？
- `connections_` 为什么不是连接池？
