#pragma once

#include <netinet/in.h>
#include <sys/socket.h>

namespace net {

class Socket {
public:
    Socket() noexcept;
    explicit Socket(int fd) noexcept;
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    static Socket create_tcp_nonblocking();

    int fd() const noexcept;
    bool valid() const noexcept;

    int release() noexcept;
    void reset(int fd = -1) noexcept;
    void close() noexcept;

    void bind(const sockaddr_in& addr) const;
    void listen(int backlog = SOMAXCONN) const;
    Socket accept(sockaddr_in* peer_addr = nullptr) const;

    void set_reuse_addr(bool on) const;
    void set_reuse_port(bool on) const;
    void set_non_blocking(bool on) const;

private:
    int fd_;
};

}  // namespace net
