#pragma once

#include <cstdint>
#include <functional>

#include "net/channel.h"
#include "net/socket.h"

namespace net {

class EventLoop;

// 监听连接的组件：管理 listen socket，并在 listen fd 可读时 accept 新连接。
class Acceptor {
public:
    // Socket 按值传递，用移动语义把已连接 socket 的所有权交给上层。
    using NewConnectionCallback = std::function<void(Socket, const sockaddr_in&)>;

    Acceptor(EventLoop* loop, uint16_t port);
    ~Acceptor();

    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

    void set_new_connection_callback(NewConnectionCallback callback);
    void listen();
    bool listening() const noexcept;

private:
    // 非阻塞地取完内核已完成连接队列中的连接。
    void handle_read();

private:
    // listen_socket_ 拥有 listen fd，accept_channel_ 只负责监听该 fd。
    Socket listen_socket_;
    Channel accept_channel_;
    NewConnectionCallback new_connection_callback_;
    bool listening_;
};

}  // namespace net
