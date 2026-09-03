#pragma once

#include <netinet/in.h>
#include <sys/socket.h>

namespace net {

// 文件描述符的 RAII 封装：对象销毁时自动关闭 fd。
// Socket 独占 fd 所有权，因此禁止拷贝，只允许通过移动转移所有权。
class Socket {
public:
    Socket() noexcept;
    explicit Socket(int fd) noexcept;
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    // 创建适合 Reactor 使用的非阻塞 TCP socket，并设置 close-on-exec。
    static Socket create_tcp_nonblocking();

    int fd() const noexcept;
    bool valid() const noexcept;

    // 释放所有权但不关闭 fd，调用方从此负责关闭它。
    int release() noexcept;
    void reset(int fd = -1) noexcept;
    void close() noexcept;

    void bind(const sockaddr_in& addr) const;
    void listen(int backlog = SOMAXCONN) const;
    // 接收一个非阻塞连接；当前没有待接收连接时返回无效 Socket。
    Socket accept(sockaddr_in* peer_addr = nullptr) const;

    void set_reuse_addr(bool on) const;
    void set_reuse_port(bool on) const;
    void set_non_blocking(bool on) const;

private:
    // fd_ < 0 表示当前对象不持有有效文件描述符。
    int fd_;
};

}  // namespace net
