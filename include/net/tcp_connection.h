#pragma once

#include <functional>
#include <memory>
#include <string>

#include "buffer/buffer.h"
#include "net/channel.h"
#include "net/socket.h"

namespace net {

class EventLoop;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using Ptr = std::shared_ptr<TcpConnection>;
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

    void connect_established();
    void send(const std::string& message);

private:
    void handle_read();
    void handle_write();
    void handle_close();
    void handle_error();

private:
    EventLoop* loop_;
    Socket socket_;
    Channel channel_;
    buffer::Buffer input_buffer_;
    buffer::Buffer output_buffer_;
    MessageCallback message_callback_;
    CloseCallback close_callback_;
    bool connected_;
};

}  // namespace net
