#pragma once

#include <cstdint>
#include <functional>

#include "net/channel.h"
#include "net/socket.h"

namespace net {

class EventLoop;

class Acceptor {
public:
    using NewConnectionCallback = std::function<void(Socket, const sockaddr_in&)>;

    Acceptor(EventLoop* loop, uint16_t port);
    ~Acceptor();

    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

    void set_new_connection_callback(NewConnectionCallback callback);
    void listen();
    bool listening() const noexcept;

private:
    void handle_read();

private:
    Socket listen_socket_;
    Channel accept_channel_;
    NewConnectionCallback new_connection_callback_;
    bool listening_;
};

}  // namespace net
