#pragma once

#include <functional>
#include <memory>
#include <string>

#include "buffer/buffer.h"
#include "net/channel.h"
#include "net/socket.h"

namespace net {

class EventLoop;

// 表示一条已建立的 TCP 连接，负责连接 fd 的读写、缓冲和关闭事件。
// 对象由 shared_ptr 管理，以保证异步事件回调执行期间连接仍然有效。
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using Ptr = std::shared_ptr<TcpConnection>;
    // 当前版本把一次可读事件中取到的全部字节作为 message，尚未划分应用层消息边界。
    using MessageCallback = std::function<void(const Ptr&, const std::string&)>;
    using CloseCallback = std::function<void(int)>;

    TcpConnection(EventLoop* loop, Socket socket);
    ~TcpConnection();

    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    int fd() const noexcept;
    bool connected() const noexcept;

    void set_message_callback(MessageCallback callback);
    void set_close_callback(CloseCallback callback);

    // 完成回调绑定并将连接 fd 的读事件注册到 EventLoop。
    void connect_established();
    // 只能在 EventLoop 线程调用；未写完的数据会进入 output_buffer_。
    void send(const std::string& message);

private:
    void handle_read();
    void handle_write();
    void handle_close();
    void handle_error();

private:
    // 非拥有指针：EventLoop 的生命周期必须长于连接。
    EventLoop* loop_;
    // socket_ 拥有连接 fd，channel_ 只负责监听该 fd 的事件。
    Socket socket_;
    Channel channel_;
    // 输入缓冲保存 socket 读到的数据，输出缓冲保存暂时未写完的数据。
    buffer::Buffer input_buffer_;
    buffer::Buffer output_buffer_;
    MessageCallback message_callback_;
    CloseCallback close_callback_;
    bool connected_;
};

}  // namespace net
