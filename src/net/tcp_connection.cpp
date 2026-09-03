#include "net/tcp_connection.h"

#include <cerrno>
#include <unistd.h>

#include <stdexcept>
#include <utility>

#include "net/event_loop.h"

namespace net {

TcpConnection::TcpConnection(EventLoop* loop, Socket socket)
    : loop_(loop),
      // 接管 Acceptor 接收到的连接 fd。
      socket_(std::move(socket)),
      channel_(loop, socket_.fd()),
      connected_(false) {
    if (loop_ == nullptr) {
        throw std::invalid_argument("TcpConnection requires a valid EventLoop");
    }
}

TcpConnection::~TcpConnection() = default;

int TcpConnection::fd() const noexcept {
    return socket_.fd();
}

bool TcpConnection::connected() const noexcept {
    return connected_;
}

void TcpConnection::set_message_callback(MessageCallback callback) {
    message_callback_ = std::move(callback);
}

void TcpConnection::set_close_callback(CloseCallback callback) {
    close_callback_ = std::move(callback);
}

void TcpConnection::connect_established() {
    loop_->assert_in_loop_thread();

    if (connected_) {
        return;
    }

    connected_ = true;

    // Channel 属于 TcpConnection；使用 weak_ptr 避免“连接 -> Channel 回调 -> 连接”的引用环。
    std::weak_ptr<TcpConnection> weak_self = shared_from_this();

    channel_.set_read_callback([weak_self]() {
        if (auto self = weak_self.lock()) {
            self->handle_read();
        }
    });
    channel_.set_write_callback([weak_self]() {
        if (auto self = weak_self.lock()) {
            self->handle_write();
        }
    });
    channel_.set_close_callback([weak_self]() {
        if (auto self = weak_self.lock()) {
            self->handle_close();
        }
    });
    channel_.set_error_callback([weak_self]() {
        if (auto self = weak_self.lock()) {
            self->handle_error();
        }
    });

    // 从此连接 fd 正式进入 Reactor，后续可读事件会进入 handle_read()。
    channel_.enable_reading();
}

void TcpConnection::send(const std::string& message) {
    loop_->assert_in_loop_thread();

    if (!connected_ || message.empty()) {
        return;
    }

    std::size_t written = 0;

    // output buffer 为空时先尝试直接写，减少一次内存拷贝和一次 EPOLLOUT 通知。
    if (output_buffer_.readable_bytes() == 0) {
        ssize_t n = ::write(socket_.fd(), message.data(), message.size());
        if (n >= 0) {
            written = static_cast<std::size_t>(n);
        } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            handle_error();
            return;
        }
    }

    if (written < message.size()) {
        // 非阻塞 write 不保证写完；剩余部分按原顺序缓存，等待 fd 再次可写。
        output_buffer_.append(message.data() + written, message.size() - written);
        channel_.enable_writing();
    }
}

void TcpConnection::handle_read() {
    if (!connected_) {
        return;
    }

    char data[4096];

    // 一次可读通知可能积累了多段数据，循环读取直到内核缓冲区暂时为空。
    while (true) {
        ssize_t n = ::read(socket_.fd(), data, sizeof(data));
        if (n > 0) {
            input_buffer_.append(data, static_cast<std::size_t>(n));
            continue;
        }

        if (n == 0) {
            // read 返回 0 表示对端已经完成有序关闭。
            handle_close();
            return;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 非阻塞 fd 的正常结束条件，本轮数据已经取完。
            break;
        }

        handle_error();
        return;
    }

    if (input_buffer_.readable_bytes() > 0 && message_callback_) {
        // v0.6 暂未实现应用层协议，本轮累积的字节整体交给 Echo 回调。
        std::string message = input_buffer_.retrieve_all_as_string();
        message_callback_(shared_from_this(), message);
    }
}

void TcpConnection::handle_write() {
    if (!connected_ || output_buffer_.readable_bytes() == 0) {
        // 没有待发送数据时必须取消 EPOLLOUT，防止 EventLoop 因持续可写而空转。
        channel_.disable_writing();
        return;
    }

    ssize_t n = ::write(socket_.fd(),
                        output_buffer_.peek(),
                        output_buffer_.readable_bytes());
    if (n > 0) {
        // 只消费实际写入内核发送缓冲区的部分，剩余数据等待下次可写事件。
        output_buffer_.retrieve(static_cast<std::size_t>(n));
        if (output_buffer_.readable_bytes() == 0) {
            channel_.disable_writing();
        }
        return;
    }

    if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        handle_error();
    }
}

void TcpConnection::handle_close() {
    if (!connected_) {
        return;
    }

    connected_ = false;
    // 先从 epoll 中取消事件，再通知 TcpServer 从活跃连接表移除本对象。
    channel_.disable_all();

    if (close_callback_) {
        close_callback_(socket_.fd());
    }
}

void TcpConnection::handle_error() {
    handle_close();
}

}  // namespace net
