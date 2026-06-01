#include "net/event_loop.h"

#include <stdexcept>

#include "net/channel.h"

namespace net {
namespace {

constexpr int kPollTimeoutMs = 10000;

}  // namespace

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      thread_id_(std::this_thread::get_id()),
      poller_() {}

EventLoop::~EventLoop() {
    if (looping_) {
        quit();
    }
}

void EventLoop::loop() {
    assert_in_loop_thread();

    if (looping_) {
        throw std::logic_error("EventLoop::loop() is already running");
    }

    looping_ = true;
    quit_.store(false);

    while (!quit_.load()) {
        active_channels_.clear();
        poller_.poll(kPollTimeoutMs, &active_channels_);

        for (Channel* channel : active_channels_) {
            if (channel != nullptr) {
                channel->handle_event();
            }
        }
    }

    looping_ = false;
}

void EventLoop::quit() {
    quit_.store(true);
}

void EventLoop::update_channel(Channel* channel) {
    assert_in_loop_thread();
    poller_.update_channel(channel);
}

void EventLoop::remove_channel(Channel* channel) {
    assert_in_loop_thread();
    poller_.remove_channel(channel);
}

bool EventLoop::is_in_loop_thread() const {
    return thread_id_ == std::this_thread::get_id();
}

void EventLoop::assert_in_loop_thread() const {
    if (!is_in_loop_thread()) {
        throw std::logic_error("EventLoop operation called from another thread");
    }
}

}  // namespace net
