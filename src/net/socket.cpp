#include "net/socket.h"

#include <cerrno>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unistd.h>

namespace net {
namespace {

void ensure_valid_fd(int fd, const char* operation) {
    if (fd < 0) {
        throw std::logic_error(std::string(operation) + " on invalid socket");
    }
}

void throw_errno(const char* operation) {
    throw std::system_error(errno, std::generic_category(), operation);
}

}  // namespace

Socket::Socket() noexcept
    : fd_(-1) {}

Socket::Socket(int fd) noexcept
    : fd_(fd) {}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept
    : fd_(other.release()) {}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        reset(other.release());
    }
    return *this;
}

Socket Socket::create_tcp_nonblocking() {
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (fd < 0) {
        throw_errno("socket");
    }

    return Socket(fd);
}

int Socket::fd() const noexcept {
    return fd_;
}

bool Socket::valid() const noexcept {
    return fd_ >= 0;
}

int Socket::release() noexcept {
    int old_fd = fd_;
    fd_ = -1;
    return old_fd;
}

void Socket::reset(int fd) noexcept {
    if (fd_ != fd) {
        close();
        fd_ = fd;
    }
}

void Socket::close() noexcept {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

void Socket::bind(const sockaddr_in& addr) const {
    ensure_valid_fd(fd_, "bind");

    if (::bind(fd_, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw_errno("bind");
    }
}

void Socket::listen(int backlog) const {
    ensure_valid_fd(fd_, "listen");

    if (::listen(fd_, backlog) < 0) {
        throw_errno("listen");
    }
}

Socket Socket::accept(sockaddr_in* peer_addr) const {
    ensure_valid_fd(fd_, "accept");

    sockaddr_in addr{};
    socklen_t addr_len = sizeof(addr);

    int conn_fd = ::accept4(fd_,
                           reinterpret_cast<sockaddr*>(&addr),
                           &addr_len,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (conn_fd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ||
            errno == ECONNABORTED) {
            return Socket();
        }
        throw_errno("accept4");
    }

    if (peer_addr != nullptr) {
        *peer_addr = addr;
    }

    return Socket(conn_fd);
}

void Socket::set_reuse_addr(bool on) const {
    ensure_valid_fd(fd_, "setsockopt(SO_REUSEADDR)");

    int opt = on ? 1 : 0;
    if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw_errno("setsockopt(SO_REUSEADDR)");
    }
}

void Socket::set_reuse_port(bool on) const {
    ensure_valid_fd(fd_, "setsockopt(SO_REUSEPORT)");

#ifdef SO_REUSEPORT
    int opt = on ? 1 : 0;
    if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        throw_errno("setsockopt(SO_REUSEPORT)");
    }
#else
    if (on) {
        throw std::runtime_error("SO_REUSEPORT is not supported on this platform");
    }
#endif
}

void Socket::set_non_blocking(bool on) const {
    ensure_valid_fd(fd_, "fcntl(O_NONBLOCK)");

    int flags = ::fcntl(fd_, F_GETFL, 0);
    if (flags < 0) {
        throw_errno("fcntl(F_GETFL)");
    }

    if (on) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    if (::fcntl(fd_, F_SETFL, flags) < 0) {
        throw_errno("fcntl(F_SETFL)");
    }
}

}  // namespace net
