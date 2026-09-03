#include "net/acceptor.h"

#include <arpa/inet.h>

#include <utility>

namespace net {
namespace {

sockaddr_in make_listen_address(uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    // INADDR_ANY 表示监听本机所有 IPv4 网卡；端口和地址都转换为网络字节序。
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    return addr;
}

}  // namespace

Acceptor::Acceptor(EventLoop* loop, uint16_t port)
    : listen_socket_(Socket::create_tcp_nonblocking()),
      accept_channel_(loop, listen_socket_.fd()),
      listening_(false) {
    // SO_REUSEADDR 必须在 bind 前设置，方便服务端快速重启并重新绑定端口。
    listen_socket_.set_reuse_addr(true);
    listen_socket_.bind(make_listen_address(port));

    accept_channel_.set_read_callback([this]() {
        handle_read();
    });
}

Acceptor::~Acceptor() {
    if (listening_) {
        accept_channel_.disable_all();
    }
}

void Acceptor::set_new_connection_callback(NewConnectionCallback callback) {
    new_connection_callback_ = std::move(callback);
}

void Acceptor::listen() {
    if (listening_) {
        return;
    }

    listen_socket_.listen();
    listening_ = true;
    // listen fd 可读表示已完成连接队列中出现了可 accept 的连接。
    accept_channel_.enable_reading();
}

bool Acceptor::listening() const noexcept {
    return listening_;
}

void Acceptor::handle_read() {
    // 一次 epoll 通知可能对应多个新连接，循环 accept 直到返回无效 Socket。
    while (true) {
        sockaddr_in peer_addr{};
        Socket connection = listen_socket_.accept(&peer_addr);
        if (!connection.valid()) {
            return;
        }

        if (new_connection_callback_) {
            // 移动 Socket，把连接 fd 的所有权交给 TcpServer/TcpConnection。
            new_connection_callback_(std::move(connection), peer_addr);
        }
    }
}

}  // namespace net
