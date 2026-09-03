#include "net/channel.h"

#include <sys/epoll.h>

#include <stdexcept>
#include <utility>

#include "net/event_loop.h"

namespace net {
namespace {

constexpr uint32_t kNoneEvent = 0;
// EPOLLPRI 表示带外或高优先级数据，也归入“可读”事件处理。
constexpr uint32_t kReadEvent = EPOLLIN | EPOLLPRI;
constexpr uint32_t kWriteEvent = EPOLLOUT;

}  // namespace

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(kNoneEvent),
      revents_(kNoneEvent),
      in_epoll_(false) {
    if (loop_ == nullptr) {
        throw std::invalid_argument("Channel requires a valid EventLoop");
    }
    if (fd_ < 0) {
        throw std::invalid_argument("Channel requires a valid fd");
    }
}

Channel::~Channel() = default;

void Channel::handle_event() {
    // 只有 HUP、没有可读数据时直接按关闭处理；若仍可读，要先让上层取完数据。
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (close_callback_) {
            close_callback_();
        }
        return;
    }

    // 同一轮可能同时出现多个事件，因此除纯 HUP 外不提前返回。
    if (revents_ & EPOLLERR) {
        if (error_callback_) {
            error_callback_();
        }
    }

    // EPOLLRDHUP 表示对端关闭了写方向，read 会读完剩余数据后返回 0。
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (read_callback_) {
            read_callback_();
        }
    }

    if (revents_ & EPOLLOUT) {
        if (write_callback_) {
            write_callback_();
        }
    }
}

void Channel::set_read_callback(EventCallback callback) {
    read_callback_ = std::move(callback);
}

void Channel::set_write_callback(EventCallback callback) {
    write_callback_ = std::move(callback);
}

void Channel::set_close_callback(EventCallback callback) {
    close_callback_ = std::move(callback);
}

void Channel::set_error_callback(EventCallback callback) {
    error_callback_ = std::move(callback);
}

int Channel::fd() const noexcept {
    return fd_;
}

uint32_t Channel::events() const noexcept {
    return events_;
}

void Channel::set_revents(uint32_t revents) noexcept {
    revents_ = revents;
}

bool Channel::is_none_event() const noexcept {
    return events_ == kNoneEvent;
}

bool Channel::is_in_epoll() const noexcept {
    return in_epoll_;
}

void Channel::set_in_epoll(bool in_epoll) noexcept {
    in_epoll_ = in_epoll;
}

void Channel::enable_reading() {
    events_ |= kReadEvent;
    update();
}

void Channel::enable_writing() {
    // 仅在确实存在待发送数据时监听 EPOLLOUT，否则可写事件会持续触发。
    events_ |= kWriteEvent;
    update();
}

void Channel::disable_writing() {
    events_ &= ~kWriteEvent;
    update();
}

void Channel::disable_all() {
    events_ = kNoneEvent;
    // Poller 会把无关注事件的 Channel 从 epoll 中删除。
    update();
}

void Channel::remove() {
    loop_->remove_channel(this);
}

void Channel::update() {
    loop_->update_channel(this);
}

}  // namespace net
