#pragma once

#include <atomic>
#include <thread>
#include <vector>

#include "net/epoll_poller.h"

namespace net {

class Channel;

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    void loop();
    void quit();

    void update_channel(Channel* channel);
    void remove_channel(Channel* channel);

    bool is_in_loop_thread() const;
    void assert_in_loop_thread() const;

private:
    bool looping_;
    std::atomic_bool quit_;
    const std::thread::id thread_id_;
    EpollPoller poller_;
    std::vector<Channel*> active_channels_;
};

}  // namespace net
