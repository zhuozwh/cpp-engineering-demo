#pragma once

#include <atomic>
#include <thread>
#include <vector>

#include "net/epoll_poller.h"

namespace net {

class Channel;

// 单线程 Reactor 的事件循环。
// Channel 的注册、移除和事件分发都必须发生在创建 EventLoop 的线程中。
class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    // 反复执行 epoll_wait，并把活跃事件分发给对应 Channel。
    void loop();
    // 设置退出标志；当前版本没有 eventfd，不能立即唤醒阻塞中的 epoll_wait。
    void quit();

    void update_channel(Channel* channel);
    void remove_channel(Channel* channel);

    bool is_in_loop_thread() const;
    void assert_in_loop_thread() const;

private:
    bool looping_;
    // quit() 可能由其他线程调用，因此退出标志使用原子变量。
    std::atomic_bool quit_;
    // 用于落实 one loop per thread 的线程约束。
    const std::thread::id thread_id_;
    EpollPoller poller_;
    // 仅保存当前一轮 poll 返回的非拥有 Channel 指针。
    std::vector<Channel*> active_channels_;
};

}  // namespace net
