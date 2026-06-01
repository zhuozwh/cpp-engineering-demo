# Reactor Core 代码阅读笔记

本文档只解释 v0.5 Reactor Core 的代码结构、文件职责和函数作用。面试回答模板维护在 `docs/PROJECT_INTERVIEW_GUIDE.md`。

当前 Reactor Core 是一个最小可运行版本，用 `timerfd` 验证事件注册、`epoll_wait`、事件分发和回调退出流程。它还不是完整 TCP Server。

---

## 1. 总体流程

`reactor_demo` 的事件流：

```text
create_timer_fd()
    ↓
创建一个非阻塞 timerfd
    ↓
EventLoop loop
    ↓
Channel timer_channel(&loop, timer_fd)
    ↓
set_read_callback(...)
    ↓
enable_reading()
    ↓
EventLoop::update_channel()
    ↓
EpollPoller::update_channel()
    ↓
epoll_ctl(EPOLL_CTL_ADD)
    ↓
loop.loop()
    ↓
epoll_wait()
    ↓
Channel::handle_event()
    ↓
read_callback()
    ↓
read(timerfd)
    ↓
tick_count >= 3
    ↓
disable_all()
    ↓
loop.quit()
```

核心分工：

- `Socket`：管理 socket fd 的生命周期和常用 socket 系统调用。
- `Channel`：描述一个 fd 关心哪些事件，以及事件发生后调用哪个回调。
- `EpollPoller`：封装 Linux `epoll`，负责注册、修改、删除、等待事件。
- `EventLoop`：事件循环，负责从 poller 获取活跃事件并分发给 Channel。
- `reactor_demo`：最小验证程序，用 `timerfd` 模拟一个会定期触发可读事件的 fd。

---

## 2. examples/reactor_demo.cpp

这个文件用于验证 Reactor Core 是否能跑通。它不创建 TCP 连接，只用 Linux `timerfd` 产生可读事件。

### FdGuard

作用：简单 RAII fd 管理类，保证 `timerfd` 在离开作用域时被 `close`。

函数：

- `FdGuard(int fd)`：保存传入 fd。
- `~FdGuard()`：如果 fd 有效，调用 `close(fd)`。
- `get()`：返回内部 fd。

为什么需要：demo 中 `timerfd_create` 返回的是裸 fd，如果中途抛异常或函数返回，需要保证 fd 被关闭。

### create_timer_fd()

作用：创建一个每 100ms 触发一次的 `timerfd`。

关键点：

- `timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)`
  - `TFD_NONBLOCK`：非阻塞，适合 Reactor。
  - `TFD_CLOEXEC`：进程执行新程序时自动关闭 fd。
- `timerfd_settime(...)`
  - 设置首次触发时间和周期触发间隔。

失败处理：

- 创建失败时抛 `std::system_error`。
- 设置 timer 失败时先关闭 fd，再抛异常。

### main()

执行流程：

1. 创建 `timerfd`。
2. 创建 `EventLoop`。
3. 创建 `Channel`，把 `timerfd` 和 `EventLoop` 绑定。
4. 给 `Channel` 设置读回调。
5. 调用 `enable_reading()`，把读事件注册到 epoll。
6. 调用 `loop.loop()` 进入事件循环。
7. timerfd 可读后触发回调。
8. 回调中读取 timerfd，累计 tick。
9. tick 到 3 后禁用事件并退出 loop。

读回调做了什么：

- 调用 `read(timerfd, &expirations, sizeof(expirations))` 消费事件。
- 如果是 `EAGAIN` 或 `EWOULDBLOCK`，说明暂时没有数据，直接返回。
- 其它 read 错误抛异常。
- tick 达到 3 后：
  - `timer_channel.disable_all()`：取消当前 Channel 关注的事件。
  - `loop.quit()`：通知 EventLoop 退出。

---

## 3. include/net/channel.h 和 src/net/channel.cpp

`Channel` 是 fd 的事件抽象。它不拥有 fd，只记录这个 fd 关心哪些事件，以及事件发生时执行哪些回调。

### 关键成员

- `EventLoop* loop_`
  - 当前 Channel 所属的 EventLoop。
- `const int fd_`
  - Channel 关注的 fd。
- `uint32_t events_`
  - 希望 epoll 监听的事件，例如 `EPOLLIN`、`EPOLLOUT`。
- `uint32_t revents_`
  - epoll 实际返回的就绪事件。
- `bool in_epoll_`
  - 当前 Channel 是否已经注册到 epoll。
- `read_callback_`
  - fd 可读时执行。
- `write_callback_`
  - fd 可写时执行。
- `close_callback_`
  - fd 关闭时执行。
- `error_callback_`
  - fd 出错时执行。

### 构造函数 Channel(EventLoop* loop, int fd)

作用：

- 绑定 `EventLoop` 和 fd。
- 初始化事件为空。
- 初始化 `in_epoll_ = false`。

防御性检查：

- `loop == nullptr` 时抛 `invalid_argument`。
- `fd < 0` 时抛 `invalid_argument`。

### handle_event()

作用：根据 `revents_` 分发回调。

执行顺序：

1. 如果收到 `EPOLLHUP` 且没有 `EPOLLIN`，说明连接挂起并且没有数据可读，调用关闭回调后返回。
2. 如果有 `EPOLLERR`，调用错误回调。
3. 如果有 `EPOLLIN | EPOLLPRI | EPOLLRDHUP`，调用读回调。
4. 如果有 `EPOLLOUT`，调用写回调。

为什么读事件里包含 `EPOLLRDHUP`：

- 对端半关闭时可能需要通过读回调处理关闭流程。

### set_read_callback / set_write_callback / set_close_callback / set_error_callback

作用：设置对应事件的回调函数。

实现细节：

- 使用 `std::move(callback)` 保存回调，避免不必要拷贝。

### fd()

作用：返回当前 Channel 对应的 fd。

### events()

作用：返回当前希望 epoll 监听的事件集合。

### set_revents(uint32_t revents)

作用：由 `EpollPoller` 在 `epoll_wait` 返回后设置实际发生的事件。

### is_none_event()

作用：判断当前是否没有关注任何事件。

使用场景：

- 如果没有任何关注事件，`EpollPoller` 可以把这个 Channel 从 epoll 中删除。

### is_in_epoll() / set_in_epoll(bool)

作用：记录当前 Channel 是否已经注册到 epoll。

为什么需要：

- 第一次注册要用 `EPOLL_CTL_ADD`。
- 已注册后修改事件要用 `EPOLL_CTL_MOD`。
- 删除事件要用 `EPOLL_CTL_DEL`。

### enable_reading()

作用：让 Channel 关注读事件。

流程：

```text
events_ |= EPOLLIN | EPOLLPRI
    ↓
update()
    ↓
EventLoop::update_channel(this)
```

### enable_writing()

作用：让 Channel 关注写事件。

### disable_writing()

作用：取消写事件关注，但保留其它事件。

### disable_all()

作用：取消当前 Channel 的所有事件。

在 demo 中用于退出前从 epoll 中删除 timerfd 事件。

### remove()

作用：显式从 EventLoop/Poller 中移除 Channel。

### update()

作用：把 Channel 的事件变化交给 EventLoop，再由 EventLoop 转给 EpollPoller。

为什么 Channel 不直接操作 EpollPoller：

- Channel 只表达事件意图。
- EventLoop 管理事件循环和 Poller。
- EpollPoller 负责平台相关的 epoll 操作。

---

## 4. include/net/event_loop.h 和 src/net/event_loop.cpp

`EventLoop` 是 Reactor 的主循环。它负责等待事件、收集活跃 Channel、调用 Channel 的事件处理函数。

### 关键成员

- `bool looping_`
  - 当前 EventLoop 是否正在运行。
- `std::atomic_bool quit_`
  - 是否请求退出循环。
- `const std::thread::id thread_id_`
  - 创建 EventLoop 的线程 id。
- `EpollPoller poller_`
  - 底层 epoll 封装。
- `std::vector<Channel*> active_channels_`
  - 本轮 `epoll_wait` 返回的活跃 Channel。

### EventLoop()

作用：

- 初始化 `looping_ = false`。
- 初始化 `quit_ = false`。
- 记录当前线程 id。
- 构造 `EpollPoller`。

### ~EventLoop()

作用：

- 如果析构时 loop 还在运行，调用 `quit()`。

注意：

- 当前版本没有跨线程 wakeup 机制。如果另一个线程调用 `quit()`，`epoll_wait` 可能要等到超时或有事件发生才返回。

### loop()

作用：启动事件循环。

流程：

```text
assert_in_loop_thread()
    ↓
检查是否已经在 loop
    ↓
looping_ = true
quit_ = false
    ↓
while (!quit_)
    ↓
active_channels_.clear()
    ↓
poller_.poll(timeout, &active_channels_)
    ↓
遍历 active_channels_
    ↓
channel->handle_event()
    ↓
looping_ = false
```

关键点：

- `loop()` 必须在创建 EventLoop 的线程调用。
- 每次循环前清空 `active_channels_`。
- `poller_.poll()` 内部阻塞在 `epoll_wait`。
- 事件分发由 `Channel::handle_event()` 完成。

### quit()

作用：设置退出标志。

在 demo 中，读回调 tick 到 3 后调用 `loop.quit()`。

### update_channel(Channel* channel)

作用：把 Channel 的事件变化交给 `EpollPoller`。

调用来源：

- `Channel::enable_reading()`
- `Channel::enable_writing()`
- `Channel::disable_writing()`
- `Channel::disable_all()`

### remove_channel(Channel* channel)

作用：把 Channel 从 poller 中移除。

### is_in_loop_thread()

作用：判断当前调用线程是否是 EventLoop 所属线程。

### assert_in_loop_thread()

作用：如果跨线程调用 EventLoop 的关键方法，则抛 `logic_error`。

为什么需要：

- 当前版本是单线程 Reactor 模型。
- 事件注册、删除、分发都要求在同一个 loop 线程执行。
- 后续如果要支持跨线程投递任务，需要增加 wakeup fd 或 eventfd。

---

## 5. include/net/epoll_poller.h 和 src/net/epoll_poller.cpp

`EpollPoller` 是 Linux epoll 的封装层。

### 关键成员

- `int epoll_fd_`
  - `epoll_create1` 返回的 epoll 实例 fd。
- `std::vector<epoll_event> events_`
  - `epoll_wait` 使用的事件数组。
- `std::unordered_map<int, Channel*> channels_`
  - fd 到 Channel 的映射。

### EpollPoller()

作用：

- 调用 `epoll_create1(EPOLL_CLOEXEC)` 创建 epoll fd。
- 初始化事件数组大小为 16。

失败处理：

- 创建失败时抛 `std::system_error`。

### ~EpollPoller()

作用：关闭 `epoll_fd_`。

### poll(int timeout_ms, std::vector<Channel*>* active_channels)

作用：等待事件，把活跃 Channel 填入 `active_channels`。

流程：

```text
检查 active_channels 非空
    ↓
epoll_wait(epoll_fd_, events_.data(), events_.size(), timeout_ms)
    ↓
如果 EINTR，直接返回
    ↓
如果其它错误，抛 system_error
    ↓
遍历返回的 epoll_event
    ↓
取出 event.data.ptr 中的 Channel*
    ↓
channel->set_revents(event.events)
    ↓
active_channels->push_back(channel)
    ↓
如果事件数组满了，扩容一倍
```

为什么 `event.data.ptr` 里存 `Channel*`：

- epoll 返回的是事件，不知道业务对象。
- 注册时把 `Channel*` 放进去，返回时就能直接找到对应 Channel。

### update_channel(Channel* channel)

作用：根据 Channel 状态决定 add、modify、delete。

逻辑：

- 如果 Channel 还没进 epoll：
  - 没有关注事件：直接返回。
  - 有关注事件：`EPOLL_CTL_ADD`。
- 如果 Channel 已经进 epoll：
  - 没有关注事件：`remove_channel()`。
  - 有关注事件：`EPOLL_CTL_MOD`。

### remove_channel(Channel* channel)

作用：从 epoll 和 `channels_` 中移除 Channel。

流程：

```text
如果 channel->is_in_epoll()
    ↓
epoll_ctl(EPOLL_CTL_DEL)
    ↓
channel->set_in_epoll(false)
    ↓
channels_.erase(fd)
    ↓
channel->set_revents(0)
```

### update(int operation, Channel* channel)

作用：实际调用 `epoll_ctl`。

它会构造：

```cpp
epoll_event event{};
event.events = channel->events();
event.data.ptr = channel;
```

然后调用：

```cpp
epoll_ctl(epoll_fd_, operation, channel->fd(), &event)
```

---

## 6. include/net/socket.h 和 src/net/socket.cpp

`Socket` 是 socket fd 的 RAII 封装。当前 `reactor_demo` 没有用它，但 v0.6 的 `Acceptor` 和 `TcpConnection` 会用到。

### 关键成员

- `int fd_`
  - 保存 socket fd。
  - `-1` 表示无效。

### Socket()

作用：构造一个无效 Socket，`fd_ = -1`。

使用场景：

- `accept()` 遇到暂时无连接时返回一个无效 Socket。

### Socket(int fd)

作用：接管一个已有 fd。

### ~Socket()

作用：析构时调用 `close()`，自动释放 fd。

### 移动构造和移动赋值

作用：转移 fd 所有权。

为什么禁止拷贝：

- fd 是系统资源，两个 Socket 如果同时拥有同一个 fd，会导致重复 close。
- 所以 `Socket` 只能移动，不能拷贝。

### create_tcp_nonblocking()

作用：创建一个非阻塞 TCP socket。

调用：

```cpp
socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)
```

关键点：

- `SOCK_STREAM`：TCP。
- `SOCK_NONBLOCK`：非阻塞。
- `SOCK_CLOEXEC`：exec 时自动关闭。

### fd()

作用：返回内部 fd。

### valid()

作用：判断 fd 是否有效。

### release()

作用：放弃 fd 所有权并返回 fd。

调用后：

- 当前 `Socket` 的 `fd_` 变成 `-1`。
- 调用方负责关闭返回的 fd。

### reset(int fd = -1)

作用：关闭当前 fd，然后接管新 fd。

### close()

作用：如果 fd 有效，调用 `::close(fd_)` 并把 `fd_` 置为 `-1`。

### bind(const sockaddr_in& addr)

作用：把 socket 绑定到本地 IP/端口。

### listen(int backlog)

作用：把 socket 转为监听状态。

### accept(sockaddr_in* peer_addr = nullptr)

作用：接受新连接。

调用：

```cpp
accept4(fd_, ..., SOCK_NONBLOCK | SOCK_CLOEXEC)
```

返回：

- 成功：返回一个持有连接 fd 的 `Socket`。
- `EAGAIN` / `EWOULDBLOCK` / `EINTR` / `ECONNABORTED`：返回无效 Socket。
- 其它错误：抛 `system_error`。

为什么部分错误返回无效 Socket：

- 非阻塞 accept 中，`EAGAIN` 和 `EWOULDBLOCK` 表示当前没有可接受连接。
- `EINTR` 表示被信号打断。
- `ECONNABORTED` 表示客户端连接在 accept 前异常中止。
- 这些都不是程序逻辑错误，可以交给上层下次再处理。

### set_reuse_addr(bool on)

作用：设置 `SO_REUSEADDR`，方便服务重启后快速绑定同一端口。

### set_reuse_port(bool on)

作用：设置 `SO_REUSEPORT`。

注意：

- 平台不支持 `SO_REUSEPORT` 且传入 `true` 时抛异常。

### set_non_blocking(bool on)

作用：通过 `fcntl` 打开或关闭 `O_NONBLOCK`。

当前 `create_tcp_nonblocking()` 和 `accept()` 已经创建非阻塞 fd，这个函数用于补充调整已有 fd。

---

## 7. 当前实现边界

当前 v0.5 只完成 Reactor Core 的最小闭环，还没有完整 TCP 服务。

已完成：

- fd 事件抽象。
- epoll 注册、修改、删除。
- EventLoop 单线程事件循环。
- timerfd demo 验证事件触发和回调。
- Socket RAII 封装。

未完成：

- 没有 `Acceptor`，还不能监听端口并接受客户端连接。
- 没有 `TcpConnection`，还不能管理连接生命周期。
- 没有 `TcpServer`，还不能管理多连接。
- 没有跨线程唤醒 EventLoop 的 `eventfd`。
- 没有连接关闭时的完整资源回收策略。
- 没有单元测试和压测。

---

## 8. 阅读顺序

建议按这个顺序读：

1. `examples/reactor_demo.cpp`
2. `include/net/channel.h`
3. `src/net/channel.cpp`
4. `include/net/event_loop.h`
5. `src/net/event_loop.cpp`
6. `include/net/epoll_poller.h`
7. `src/net/epoll_poller.cpp`
8. `include/net/socket.h`
9. `src/net/socket.cpp`

读完后要能回答：

- 一个 fd 是怎么被注册到 epoll 的？
- epoll 返回事件后，怎么找到对应的 Channel？
- EventLoop 在循环里做了什么？
- Channel 为什么要保存 `events_` 和 `revents_` 两套事件？
- 为什么 socket 要非阻塞？
- 当前为什么还不能算 TCP Server？
