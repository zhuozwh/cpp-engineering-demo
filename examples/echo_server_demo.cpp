#include "net/event_loop.h"
#include "net/tcp_connection.h"
#include "net/tcp_server.h"

#include <cstdint>
#include <iostream>

int main() {
    constexpr uint16_t kPort = 8080;

    net::EventLoop loop;
    // TcpServer 内部组合 Acceptor，并为每个客户端创建 TcpConnection。
    net::TcpServer server(&loop, kPort);

    // 消息回调运行在 EventLoop 线程；Echo 业务只需把收到的数据原样发回。
    server.set_message_callback(
        [](const net::TcpConnection::Ptr& connection, const std::string& message) {
            connection->send(message);
        });

    // start() 完成 listen 并把 listen fd 的读事件注册到 Reactor。
    server.start();

    std::cout << "echo server listening on 0.0.0.0:" << kPort << std::endl;
    // 进入 epoll_wait 事件循环，开始处理新连接和连接上的读写事件。
    loop.loop();

    return 0;
}
