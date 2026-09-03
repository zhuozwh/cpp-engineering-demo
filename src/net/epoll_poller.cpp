#include "net/epoll_poller.h"

#include <cerrno>
#include <stdexcept>
#include <system_error>
#include <unistd.h>

#include "net/channel.h"

namespace net {
namespace {

constexpr int kInitialEventListSize = 16;

void throw_errno(const char* operation) {
    throw std::system_error(errno, std::generic_category(), operation);
}

}  // namespace

EpollPoller::EpollPoller()
    : epoll_fd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitialEventListSize) {
    if (epoll_fd_ < 0) {
        throw_errno("epoll_create1");
    }
}

EpollPoller::~EpollPoller() {
    if (epoll_fd_ >= 0) {
        ::close(epoll_fd_);
    }
}

void EpollPoller::poll(int timeout_ms, std::vector<Channel*>* active_channels) {
    if (active_channels == nullptr) {
        throw std::invalid_argument("active_channels must not be null");
    }

    // events_ 只承接本轮内核返回的就绪事件，不保存长期注册状态。
    int event_count = ::epoll_wait(epoll_fd_,
                                   events_.data(),
                                   static_cast<int>(events_.size()),
                                   timeout_ms);

    if (event_count < 0) {
        if (errno == EINTR) {
            return;
        }
        throw_errno("epoll_wait");
    }

    for (int i = 0; i < event_count; ++i) {
        // 注册时把 Channel* 放进 data.ptr，这里无需再通过 fd 查表即可取回。
        auto* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        active_channels->push_back(channel);
    }

    if (event_count == static_cast<int>(events_.size())) {
        // 结果数组被填满说明下一轮可能容纳不下，扩大容量以免分批唤醒。
        events_.resize(events_.size() * 2);
    }
}

void EpollPoller::update_channel(Channel* channel) {
    if (channel == nullptr) {
        throw std::invalid_argument("channel must not be null");
    }

    const int fd = channel->fd();

    if (!channel->is_in_epoll()) {
        if (channel->is_none_event()) {
            return;
        }

        // 首次关注事件：ADD，并同步维护用户态注册状态。
        update(EPOLL_CTL_ADD, channel);
        channels_[fd] = channel;
        channel->set_in_epoll(true);
        return;
    }

    if (channel->is_none_event()) {
        // 不再关注任何事件时直接 DEL，避免保留无意义的 epoll 项。
        remove_channel(channel);
        return;
    }

    // 已注册且仍有关注事件，只需修改事件掩码。
    update(EPOLL_CTL_MOD, channel);
}

void EpollPoller::remove_channel(Channel* channel) {
    if (channel == nullptr) {
        throw std::invalid_argument("channel must not be null");
    }

    const int fd = channel->fd();

    if (channel->is_in_epoll()) {
        update(EPOLL_CTL_DEL, channel);
        channel->set_in_epoll(false);
    }

    channels_.erase(fd);
    channel->set_revents(0);
}

void EpollPoller::update(int operation, Channel* channel) {
    epoll_event event{};
    event.events = channel->events();
    // epoll 只借用该指针；Channel 必须活到被移除之后。
    event.data.ptr = channel;

    if (::epoll_ctl(epoll_fd_, operation, channel->fd(), &event) < 0) {
        throw_errno("epoll_ctl");
    }
}

}  // namespace net
