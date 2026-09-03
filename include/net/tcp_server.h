#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "net/acceptor.h"
#include "net/tcp_connection.h"

namespace net {

class EventLoop;

// 组合 Acceptor 与 TcpConnection，负责接收连接并管理活跃连接生命周期。
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
    // 活跃连接表用于持有 TcpConnection，并不是复用外部连接的“连接池”。
    std::unordered_map<int, std::shared_ptr<TcpConnection>> connections_;
};

}  // namespace net
