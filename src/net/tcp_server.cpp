#include "net/tcp_server.h"

#include <stdexcept>
#include <utility>

#include "net/event_loop.h"

namespace net {

TcpServer::TcpServer(EventLoop* loop, uint16_t port)
    : loop_(loop),
      acceptor_(loop, port) {
    if (loop_ == nullptr) {
        throw std::invalid_argument("TcpServer requires a valid EventLoop");
    }

    acceptor_.set_new_connection_callback(
        [this](Socket socket, const sockaddr_in& peer_addr) {
            handle_new_connection(std::move(socket), peer_addr);
        });
}

TcpServer::~TcpServer() = default;

void TcpServer::set_message_callback(MessageCallback callback) {
    message_callback_ = std::move(callback);
}

void TcpServer::start() {
    loop_->assert_in_loop_thread();
    acceptor_.listen();
}

void TcpServer::handle_new_connection(Socket socket, const sockaddr_in& peer_addr) {
    (void)peer_addr;

    loop_->assert_in_loop_thread();

    const int fd = socket.fd();
    auto connection = std::make_shared<TcpConnection>(loop_, std::move(socket));

    connection->set_message_callback(message_callback_);
    connection->set_close_callback([this](int closed_fd) {
        remove_connection(closed_fd);
    });

    connections_[fd] = connection;
    connection->connect_established();
}

void TcpServer::remove_connection(int fd) {
    loop_->assert_in_loop_thread();
    connections_.erase(fd);
}

}  // namespace net
