#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "net/acceptor.h"
#include "net/tcp_connection.h"

namespace net {

class EventLoop;

class TcpServer {
public:
    using MessageCallback = TcpConnection::MessageCallback;

    TcpServer(EventLoop* loop, uint16_t port);
    ~TcpServer();

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    void set_message_callback(MessageCallback callback);
    void start();

private:
    void handle_new_connection(Socket socket, const sockaddr_in& peer_addr);
    void remove_connection(int fd);

private:
    EventLoop* loop_;
    Acceptor acceptor_;
    MessageCallback message_callback_;
    std::unordered_map<int, std::shared_ptr<TcpConnection>> connections_;
};

}  // namespace net
