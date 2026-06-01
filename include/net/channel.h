#pragma once

#include <cstdint>
#include <functional>

namespace net {

class EventLoop;

class Channel {
public:
    using EventCallback = std::function<void()>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    void handle_event();

    void set_read_callback(EventCallback callback);
    void set_write_callback(EventCallback callback);
    void set_close_callback(EventCallback callback);
    void set_error_callback(EventCallback callback);

    int fd() const noexcept;
    uint32_t events() const noexcept;
    void set_revents(uint32_t revents) noexcept;

    bool is_none_event() const noexcept;
    bool is_in_epoll() const noexcept;
    void set_in_epoll(bool in_epoll) noexcept;

    void enable_reading();
    void enable_writing();
    void disable_writing();
    void disable_all();
    void remove();

private:
    void update();

private:
    EventLoop* loop_;
    const int fd_;
    uint32_t events_;
    uint32_t revents_;
    bool in_epoll_;

    EventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
};

}  // namespace net
