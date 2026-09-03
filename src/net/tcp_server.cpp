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
            // Acceptor 只负责 accept，连接对象的创建和生命周期交给 TcpServer。
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

    // 先放入活跃连接表持有 shared_ptr，再注册事件，保证回调触发时对象仍存在。
    connections_[fd] = connection;
    connection->connect_established();
}

void TcpServer::remove_connection(int fd) {
    loop_->assert_in_loop_thread();
    // erase 释放 Server 持有的 shared_ptr；无其他引用时连接对象随即析构并关闭 fd。
    connections_.erase(fd);
}

}  // namespace net
