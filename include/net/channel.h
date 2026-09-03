#pragma once

#include <cstdint>
#include <functional>

namespace net {

class EventLoop;

// Channel 是“fd + 感兴趣的事件 + 事件回调”的轻量封装。
// 它不拥有 fd，fd 的生命周期由 Socket、timerfd 的 RAII 对象等负责。
class Channel {
public:
    using EventCallback = std::function<void()>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    // 根据本轮 epoll 返回的 revents_ 分发读、写、关闭和错误回调。
    void handle_event();

    void set_read_callback(EventCallback callback);
    void set_write_callback(EventCallback callback);
    void set_close_callback(EventCallback callback);
    void set_error_callback(EventCallback callback);

    int fd() const noexcept;
    // events_ 是希望 epoll 监听的事件；revents_ 是本轮实际发生的事件。
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
    // 将 events_ 的变化交给所属 EventLoop，再由 Poller 更新 epoll。
    void update();

private:
    // 非拥有指针：EventLoop 的生命周期必须长于 Channel。
    EventLoop* loop_;
    const int fd_;
    uint32_t events_;
    uint32_t revents_;
    // 记录该 Channel 当前是否已注册到 epoll，决定使用 ADD 还是 MOD。
    bool in_epoll_;

    EventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
};

}  // namespace net
