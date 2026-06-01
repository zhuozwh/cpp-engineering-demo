#pragma once

#include <sys/epoll.h>

#include <unordered_map>
#include <vector>

namespace net {

class Channel;

class EpollPoller {
public:
    EpollPoller();
    ~EpollPoller();

    EpollPoller(const EpollPoller&) = delete;
    EpollPoller& operator=(const EpollPoller&) = delete;

    void poll(int timeout_ms, std::vector<Channel*>* active_channels);
    void update_channel(Channel* channel);
    void remove_channel(Channel* channel);

private:
    void update(int operation, Channel* channel);

private:
    int epoll_fd_;
    std::vector<epoll_event> events_;
    std::unordered_map<int, Channel*> channels_;
};

}  // namespace net
