#include "net/channel.h"
#include "net/event_loop.h"

#include <sys/timerfd.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <iostream>
#include <system_error>

namespace {

// 示例中的最小 RAII 工具，保证 timerfd 在 main 退出时被关闭。
class FdGuard {
public:
    explicit FdGuard(int fd)
        : fd_(fd) {}

    ~FdGuard() {
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }

    FdGuard(const FdGuard&) = delete;
    FdGuard& operator=(const FdGuard&) = delete;

    int get() const noexcept {
        return fd_;
    }

private:
    int fd_;
};

int create_timer_fd() {
    // timerfd 可以像普通 fd 一样交给 epoll 监听，很适合验证 Reactor 主链路。
    int fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (fd < 0) {
        throw std::system_error(errno, std::generic_category(), "timerfd_create");
    }

    itimerspec spec{};
    // 首次 100 ms 后触发，之后每隔 100 ms 周期触发。
    spec.it_value.tv_nsec = 100 * 1000 * 1000;
    spec.it_interval.tv_nsec = 100 * 1000 * 1000;

    if (::timerfd_settime(fd, 0, &spec, nullptr) < 0) {
        int saved_errno = errno;
        ::close(fd);
        throw std::system_error(saved_errno, std::generic_category(), "timerfd_settime");
    }

    return fd;
}

}  // namespace

int main() {
    FdGuard timer_fd(create_timer_fd());

    net::EventLoop loop;
    net::Channel timer_channel(&loop, timer_fd.get());

    int tick_count = 0;

    timer_channel.set_read_callback([&]() {
        uint64_t expirations = 0;
        // 必须读取 timerfd 才能消费可读状态；expirations 还能反映累计超时次数。
        ssize_t n = ::read(timer_fd.get(), &expirations, sizeof(expirations));
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }
            throw std::system_error(errno, std::generic_category(), "read(timerfd)");
        }

        tick_count += static_cast<int>(expirations);
        std::cout << "reactor tick " << tick_count << std::endl;

        if (tick_count >= 3) {
            // 退出前先取消 Channel 的所有关注事件，使其从 epoll 中移除。
            timer_channel.disable_all();
            loop.quit();
        }
    });

    // enable_reading -> EventLoop -> EpollPoller，最终执行 epoll_ctl(ADD)。
    timer_channel.enable_reading();

    std::cout << "reactor demo started" << std::endl;
    loop.loop();
    std::cout << "reactor demo stopped" << std::endl;

    return 0;
}
