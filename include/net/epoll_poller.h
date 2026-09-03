#pragma once

#include <sys/epoll.h>

#include <unordered_map>
#include <vector>

namespace net {

class Channel;

// 对 Linux epoll 的薄封装，负责注册 Channel 并返回本轮活跃 Channel。
// Poller 不拥有 Channel；注册期间 Channel 必须保持有效。
class EpollPoller {
public:
    EpollPoller();
    ~EpollPoller();

    EpollPoller(const EpollPoller&) = delete;
    EpollPoller& operator=(const EpollPoller&) = delete;

    // 阻塞等待事件，并把结果追加到 active_channels。
    void poll(int timeout_ms, std::vector<Channel*>* active_channels);
    void update_channel(Channel* channel);
    void remove_channel(Channel* channel);

private:
    void update(int operation, Channel* channel);

private:
    int epoll_fd_;
    // epoll_wait 的结果数组；事件装满时会自动扩容。
    std::vector<epoll_event> events_;
    // 保存 fd 到 Channel 的注册关系，用于记录当前受管理的 Channel。
    std::unordered_map<int, Channel*> channels_;
};

}  // namespace net
