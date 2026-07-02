#include "net/event_loop.h"
#include "net/tcp_connection.h"
#include "net/tcp_server.h"

#include <cstdint>
#include <iostream>

int main() {
    constexpr uint16_t kPort = 8080;

    net::EventLoop loop;
    net::TcpServer server(&loop, kPort);

    server.set_message_callback(
        [](const net::TcpConnection::Ptr& connection, const std::string& message) {
            connection->send(message);
        });

    server.start();

    std::cout << "echo server listening on 0.0.0.0:" << kPort << std::endl;
    loop.loop();

    return 0;
}
