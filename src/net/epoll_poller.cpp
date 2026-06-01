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
        auto* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        active_channels->push_back(channel);
    }

    if (event_count == static_cast<int>(events_.size())) {
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

        update(EPOLL_CTL_ADD, channel);
        channels_[fd] = channel;
        channel->set_in_epoll(true);
        return;
    }

    if (channel->is_none_event()) {
        remove_channel(channel);
        return;
    }

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
    event.data.ptr = channel;

    if (::epoll_ctl(epoll_fd_, operation, channel->fd(), &event) < 0) {
        throw_errno("epoll_ctl");
    }
}

}  // namespace net
