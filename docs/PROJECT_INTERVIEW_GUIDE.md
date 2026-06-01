# cpp_service_framework 项目面试手册

这个文件专门用来维护项目复盘、面试高频问题、回答模板和每个模块背后的原理。

以后每完成一个模块，都可以追加四类内容：

- 这个模块解决什么问题
- 核心数据结构和执行流程是什么
- 面试官可能怎么追问
- 我应该怎么用自己的话讲出来

---

## 1. 项目一句话介绍

这是一个轻量级 C++ 后端服务框架学习项目，目标是从基础设施开始，逐步实现日志、线程池、网络缓冲区、配置读取、Reactor 事件循环、TCP Server、HTTP Server 等后端核心组件。

如果面试官问“你这个项目是做什么的”，可以这样回答：

> 这个项目不是直接做一个业务系统，而是自己实现一个简化版 C++ 服务端框架。它的主线是：先完成日志、线程池、Buffer、配置这些基础设施，再基于 Linux epoll 实现 Reactor 网络模型，后面继续封装 TcpServer 和 HTTPServer。我的目标是通过这个项目把 C++ 后端里的并发、IO 多路复用、网络编程和工程组织串起来。

---

## 2. 项目主线

当前项目可以按这条线讲：

```text
Logger / Config
    ↓
基础运行能力：日志输出、配置加载

ThreadPool
    ↓
并发任务执行能力

Buffer
    ↓
网络读写中的数据暂存和协议解析基础

Socket / Channel / EpollPoller / EventLoop
    ↓
Reactor 事件驱动网络核心

TcpConnection / TcpServer
    ↓
完整 TCP 服务

HTTPParser / Router / HTTPServer
    ↓
HTTP 服务 Demo
```

面试时不要只说“我写了线程池、日志、Buffer”。要说清楚它们为什么要放在一个后端框架里：

- Logger：服务运行时要能定位问题。
- Config：服务端口、线程数、日志级别不能写死在代码里。
- ThreadPool：业务任务或耗时任务不能都阻塞 IO 线程。
- Buffer：TCP 是字节流，一次读到的数据不一定是一条完整消息，需要缓存和拆包。
- Reactor：服务端要同时管理大量连接，不能一个连接一个线程，需要 epoll 监听事件。

---

## 3. 这次面试复盘：为什么项目答不上来

记录：2026-05-25 视频面试。面试官上来直接问项目，追问线程池怎么工作、Buffer 怎么工作、为什么要有 Buffer、数据怎么存入 Buffer、线程池怎么拿到任务等。回答准备不足，20 分钟左右结束。

这次暴露的问题不是单纯“基础太差”，更准确地说是三层问题叠在一起：

### 3.1 项目主线没有形成

如果只知道“写过线程池、写过 Buffer”，但说不出它们在服务端框架中的位置，面试官会觉得项目是堆模块。

需要补上的能力：

- 能用 1 分钟讲清楚项目整体目标。
- 能用 3 分钟讲清楚一次请求从 socket 到业务处理的大概链路。
- 能说明每个模块为什么存在，而不是只说明它有什么函数。

### 3.2 代码会写，但执行流程没变成语言

比如线程池，代码里有 `submit`、任务队列、`condition_variable`、worker 线程，但面试时要讲的是：

```text
主线程 submit 任务
    ↓
任务被封装成 packaged_task
    ↓
放进任务队列
    ↓
notify_one 唤醒一个 worker
    ↓
worker 从队列取任务
    ↓
释放锁并执行任务
    ↓
future 拿到返回值
```

你不是完全不会，而是没有把代码流程训练成口头表达。

### 3.3 对后端面试官的追问方向预判不足

面试官不会只问“你实现了什么”，更常问：

- 为什么需要这个模块？
- 这个模块解决了什么后端问题？
- 数据从哪里来，到哪里去？
- 多线程下怎么保证安全？
- 有没有边界情况？
- 如果压力变大，会在哪里出问题？
- 和成熟库相比，你这个简化在哪里？

以后准备项目，必须按这些追问方向准备。

---

## 4. ThreadPool：原理和面试回答

相关代码：

- `include/threadpool/thread_pool.h`
- `src/threadpool/thread_pool.cpp`
- `examples/threadpool_demo.cpp`

### 4.1 线程池解决什么问题

线程池解决的是“频繁创建和销毁线程成本高、任务并发执行需要统一管理”的问题。

服务端程序里，如果每来一个任务就创建一个线程，会有几个问题：

- 创建线程有系统调用和栈空间成本。
- 线程数量不可控，压力大时可能把系统资源打爆。
- 任务执行和线程生命周期耦合，不方便统一停止。

线程池的做法是：提前创建固定数量的 worker 线程，任务来了就放入队列，由空闲 worker 取出来执行。

### 4.2 当前实现的核心结构

```cpp
std::vector<std::thread> workers_;
std::queue<std::function<void()>> tasks_;

std::mutex mutex_;
std::condition_variable cv_;
bool stop_;
```

含义：

- `workers_`：保存工作线程。
- `tasks_`：任务队列，生产者是调用 `submit` 的线程，消费者是 worker 线程。
- `mutex_`：保护任务队列和停止标志。
- `cv_`：没有任务时让 worker 睡眠，有任务时唤醒。
- `stop_`：析构时通知 worker 退出。

### 4.3 submit 怎么工作

当前 `submit` 是模板函数，支持任意可调用对象和参数，并返回 `std::future`。

流程：

```text
submit(f, args...)
    ↓
用 std::bind 绑定函数和参数
    ↓
封装成 std::packaged_task
    ↓
通过 get_future 拿到 future
    ↓
加锁，把任务放入 tasks_ 队列
    ↓
notify_one 唤醒一个 worker
    ↓
返回 future 给调用方
```

面试回答模板：

> 我这个线程池里，调用方通过 `submit` 提交任务。`submit` 会把函数和参数封装成 `packaged_task`，然后拿到对应的 `future` 返回给调用方。真正放入队列的是一个 `std::function<void()>`，它内部会执行这个 packaged_task。worker 线程一直在循环等待任务，队列为空时通过 condition_variable 阻塞，有任务后被唤醒，从队列里取出任务，释放锁后执行。这样任务提交和任务执行就解耦了。

### 4.4 worker 怎么拿到任务

worker 在 `worker_loop` 中运行：

```text
while true
    加锁
    cv.wait(lock, stop_ || !tasks_.empty())
    如果 stop_ 且队列为空，退出线程
    从队列头部取出一个任务
    解锁
    执行任务
```

重点：执行任务时不持有锁。

如果持锁执行任务，会导致其它线程不能提交任务、其它 worker 不能取任务，线程池基本失去并发意义。

### 4.5 为什么用 condition_variable

不用 `condition_variable` 的话，worker 可能要一直循环检查队列是否有任务，这叫忙等，会浪费 CPU。

`condition_variable` 可以让 worker 在没有任务时睡眠，任务到来时由 `submit` 调用 `notify_one` 唤醒一个 worker。

### 4.6 为什么返回 future

`future` 用来让提交任务的线程拿到异步执行结果。

如果任务有返回值，调用方可以：

```cpp
auto result = pool.submit([] { return 42; });
int value = result.get();
```

`get()` 会等待任务执行完成，并拿到返回值。如果任务内部抛异常，异常也会通过 future 传回调用方。

### 4.7 高频追问

Q：线程池析构时怎么退出？

A：析构函数里先加锁把 `stop_` 设置为 true，然后 `notify_all` 唤醒所有 worker。worker 醒来后，如果发现 `stop_ == true` 且任务队列为空，就退出循环。析构函数再 `join` 所有线程，保证线程资源被回收。

Q：如果 stop 之后继续 submit 会怎样？

A：当前实现里 `submit` 会在加锁后检查 `stop_`，如果线程池已经停止，就抛出 `runtime_error`，避免任务被放入一个不再执行的队列。

Q：为什么任务队列里存的是 `std::function<void()>`？

A：因为不同任务的返回值类型可能不同，不能直接放进同一个队列。通过 `packaged_task<ReturnType()>` 保存真实返回值，再包一层 `void()` lambda 放进队列，队列类型就统一了，返回值通过 future 交给调用方。

Q：线程池适合处理什么任务？

A：适合处理 CPU 计算任务、业务逻辑任务、短时间阻塞任务。对于纯网络 IO，通常不会把每个连接直接丢进线程池阻塞读写，而是用 Reactor 监听 IO 事件，再把耗时业务投递到线程池。

---

## 5. Buffer：原理和面试回答

相关代码：

- `include/buffer/buffer.h`
- `src/buffer/buffer.cpp`
- `examples/buffer_demo.cpp`

### 5.1 为什么需要 Buffer

TCP 是字节流，不保留消息边界。

这意味着：

- 一次 `read` 可能只读到半个请求。
- 一次 `read` 也可能读到多个请求粘在一起。
- 应用层协议解析速度和 socket 收包速度不一定一致。

所以需要 Buffer 作为中间层，把从 socket 读到的字节先存起来，等数据足够时再解析；解析完一部分后，再把已处理的数据丢掉。

面试回答模板：

> Buffer 主要是为了解决 TCP 字节流没有消息边界的问题。网络读到的数据不一定刚好是一条完整请求，所以我不会直接假设一次 read 就能解析完，而是先 append 到输入缓冲区。协议解析器从 readable 区域读取数据，解析掉多少就 retrieve 多少。这样可以处理半包、粘包，也能解耦网络收包和协议解析。

### 5.2 当前 Buffer 的逻辑结构

```text
| 已读区域 | 可读区域 | 可写区域 |
0      reader_index_   writer_index_   buffer_.size()
```

核心成员：

```cpp
std::vector<char> buffer_;
std::size_t reader_index_;
std::size_t writer_index_;
```

含义：

- `reader_index_`：当前可读数据开始位置。
- `writer_index_`：当前可写位置，也是可读数据结束位置。
- `readable_bytes()`：`writer_index_ - reader_index_`
- `writable_bytes()`：`buffer_.size() - writer_index_`
- `prependable_bytes()`：`reader_index_`

### 5.3 数据怎么存入 Buffer

调用：

```cpp
buf.append(data, len);
```

流程：

```text
append(data, len)
    ↓
ensure_writable_bytes(len)
    ↓
如果尾部空间不够，make_space(len)
    ↓
把 data 拷贝到 begin() + writer_index_
    ↓
writer_index_ += len
```

### 5.4 retrieve 怎么工作

`retrieve(len)` 表示消费掉前 `len` 个可读字节。

如果只消费一部分：

```text
reader_index_ += len
```

如果消费掉全部：

```text
reader_index_ = 0
writer_index_ = 0
```

这样底层 vector 不释放，后续还能复用空间。

### 5.5 空间不够怎么办

`make_space(len)` 有两种策略：

第一种：尾部空间 + 前面已读空间仍不够，就扩容。

```text
writable_bytes() + prependable_bytes() < len
    ↓
buffer_.resize(writer_index_ + len)
```

第二种：总空间够，只是尾部不够，就把可读数据移动到开头，复用前面的已读区域。

```text
把 [reader_index_, writer_index_) 移动到 buffer_ 开头
reader_index_ = 0
writer_index_ = readable
```

### 5.6 高频追问

Q：为什么不用 string 直接拼？

A：`std::string` 也能存字节，但 Buffer 明确维护读写索引，更适合网络场景。它能区分已读、可读、可写区域，避免频繁 erase 造成移动，也方便协议解析器消费部分数据。

Q：为什么 retrieve 后不马上释放内存？

A：服务端连接会频繁读写，如果每次消费数据都释放内存，下次读又要重新分配。Buffer 保留底层空间可以复用，减少内存分配开销。

Q：Buffer 怎么处理半包？

A：如果当前 readable 数据不足以解析完整请求，就先不 retrieve，继续等待下一次 socket 可读，把新数据 append 到后面。等数据足够后再解析。

Q：Buffer 怎么处理粘包？

A：如果 readable 数据里有多条请求，解析器解析完第一条后 retrieve 对应长度，然后继续检查剩余 readable 数据是否还能解析下一条。

---

## 6. Reactor Core：原理和面试回答

相关代码：

- `include/net/socket.h`
- `include/net/channel.h`
- `include/net/epoll_poller.h`
- `include/net/event_loop.h`
- `examples/reactor_demo.cpp`

### 6.1 Reactor 解决什么问题

服务端要同时管理很多连接，如果每个连接一个线程，线程数量会很快膨胀，调度和内存成本都很高。

Reactor 模型的核心思想是：

```text
一个或少数几个 EventLoop 负责监听大量 fd 的 IO 事件
事件就绪后，再调用对应回调处理
```

在 Linux 下，底层通常用 `epoll` 实现。

### 6.2 当前几个类怎么分工

- `Socket`：负责 fd 生命周期和 socket 系统调用封装。
- `Channel`：代表一个 fd 关心什么事件，以及事件发生后调用什么回调。
- `EpollPoller`：负责和 epoll 打交道，注册、修改、删除 fd 事件。
- `EventLoop`：负责循环调用 poller，拿到活跃 Channel，然后分发事件。

流程：

```text
Channel::enable_reading()
    ↓
EventLoop::update_channel()
    ↓
EpollPoller::epoll_ctl(ADD/MOD)
    ↓
EventLoop::loop()
    ↓
EpollPoller::epoll_wait()
    ↓
Channel::handle_event()
    ↓
执行 read/write/error/close callback
```

面试回答模板：

> 我这里实现的是一个简化 Reactor。Socket 只负责 fd 和 socket 操作；Channel 是 fd 的事件抽象，里面保存关注的事件和回调；EpollPoller 封装 epoll_ctl 和 epoll_wait；EventLoop 是主循环，阻塞在 epoll_wait 上，拿到活跃事件后调用对应 Channel 的 handle_event。这样 fd 的监听和业务回调就解耦了。

### 6.3 为什么 Channel 不直接调用 epoll

这是为了职责拆分。

如果 Channel 直接调用 epoll，它既要保存事件，又要知道 epoll 的实现细节。现在 Channel 只表达“我关心读事件、写事件”，真正怎么注册给内核由 Poller 负责。

后续如果要换成其它 IO 多路复用机制，理论上只需要替换 Poller 层。

### 6.4 高频追问

Q：epoll_wait 返回后发生了什么？

A：`EpollPoller::poll` 调用 `epoll_wait`，内核返回就绪事件。每个事件里保存了对应的 `Channel*`，poller 把这些 Channel 放进 `active_channels_`。然后 `EventLoop` 遍历这些 Channel，调用 `handle_event`，最终执行读、写、关闭或错误回调。

Q：为什么要非阻塞 socket？

A：Reactor 模型里一个 EventLoop 要管理多个 fd。如果某个 fd 的 read/write 阻塞住，整个 EventLoop 都会卡住，其他连接的事件就处理不了。所以 socket 一般要设成非阻塞。

Q：EventLoop 和 ThreadPool 什么关系？

A：EventLoop 适合处理 IO 事件，要求不能长时间阻塞。ThreadPool 适合处理耗时任务。常见做法是 EventLoop 读到请求并完成基础解析后，把耗时业务投递到线程池，线程池执行完再把结果交回 EventLoop 写回连接。

### 6.5 Reactor Core 面试怎么说

面试官问“你这个 Reactor 是怎么跑起来的”，不要先背类名，先从 demo 的一次事件说起：

```text
reactor_demo 创建 timerfd
    ↓
创建 EventLoop
    ↓
用 timerfd 创建 Channel
    ↓
给 Channel 设置 read_callback
    ↓
enable_reading 注册读事件
    ↓
EventLoop 调用 epoll_wait 等事件
    ↓
timerfd 到期后 epoll 返回
    ↓
EventLoop 调用 Channel::handle_event
    ↓
Channel 执行 read_callback
    ↓
读 timerfd，tick 到 3 后退出 loop
```

可以这样回答：

> 我现在 v0.5 做的是最小 Reactor Core，不是完整 TCP Server。demo 里我用 timerfd 模拟一个会定期变成可读的 fd。程序先创建 EventLoop，再把 timerfd 包成 Channel，并设置读回调。调用 enable_reading 后，Channel 会通过 EventLoop 交给 EpollPoller 注册到 epoll。EventLoop 进入 loop 后阻塞在 epoll_wait，timerfd 到期后 epoll 返回事件，Poller 把事件转成活跃 Channel，EventLoop 调用 handle_event，最后执行我设置的 read_callback。回调里读取 timerfd，累计三次后 disable_all 并 quit，事件循环退出。

如果面试官继续问“那几个类分别有什么用”，按这个顺序说：

```text
Socket：管理 fd 生命周期和 socket 系统调用
Channel：保存一个 fd 关心的事件和回调
EpollPoller：封装 epoll_ctl 和 epoll_wait
EventLoop：事件循环，拿活跃 Channel 并分发事件
```

### 6.6 Reactor Core 常见追问应对

Q：为什么 demo 用 timerfd，不直接写 TCP？

A：timerfd 可以稳定地产生可读事件，用来验证 Reactor 的核心链路：注册 fd、等待事件、返回活跃 Channel、执行回调。TCP Server 还需要 Acceptor、TcpConnection 和连接生命周期管理，那是下一阶段 v0.6 的内容。

Q：Channel 拥有 fd 吗？

A：不拥有。Channel 只是 fd 的事件视图，保存 fd、关注事件、就绪事件和回调。fd 的生命周期由外层对象管理，比如 demo 里的 `FdGuard`，后续 TCP 里会由 `Socket` / `TcpConnection` 管理。

Q：`events_` 和 `revents_` 有什么区别？

A：`events_` 是我希望监听的事件，比如读事件或写事件；`revents_` 是 epoll 实际返回的事件。Channel 根据 `revents_` 判断本次到底发生了可读、可写、关闭还是错误。

Q：`enable_reading()` 最后为什么会走到 epoll_ctl？

A：`enable_reading()` 会修改 Channel 的 `events_`，然后调用 `update()`。`update()` 把当前 Channel 交给 EventLoop，EventLoop 再调用 EpollPoller 的 `update_channel()`。Poller 根据 Channel 是否已经在 epoll 中，选择 `EPOLL_CTL_ADD` 或 `EPOLL_CTL_MOD`。

Q：epoll 返回后怎么找到对应的 Channel？

A：注册事件时，EpollPoller 把 `Channel*` 放进 `epoll_event.data.ptr`。`epoll_wait` 返回后，再从 `data.ptr` 取回这个 `Channel*`，设置它的 `revents_`，放进 active_channels。

Q：为什么 EventLoop 要限制线程？

A：当前实现是单线程 Reactor。Channel 的注册、删除和事件分发都在创建 EventLoop 的线程执行，避免并发修改 Poller 和 Channel 状态。后续如果要跨线程调用，需要增加 eventfd 唤醒和任务队列。

Q：当前 v0.5 最大的不足是什么？

A：它只是 Reactor Core 的最小闭环，还没有 TCP 连接管理。缺少 Acceptor、TcpConnection、TcpServer、连接关闭资源回收、跨线程唤醒和测试压测。它能证明 epoll 事件分发链路是通的，但还不能对外提供 TCP 服务。

Q：这部分和后面的 TcpServer 怎么接？

A：下一步会用 `Socket` 创建 listen fd，再用 `Channel` 监听 listen fd 的可读事件。listen fd 可读表示有新连接，Acceptor 调用 accept 拿到连接 fd，再为连接 fd 创建 TcpConnection 和对应 Channel，后续读写事件就由 TcpConnection 处理。

---

## 7. 面试官常见项目追问清单

### 7.1 项目整体

Q：你这个项目解决了什么问题？

A：它是一个 C++ 后端服务框架学习项目，目标是自己实现服务端常见基础组件，理解从配置、日志、线程池、Buffer 到 Reactor 网络模型的完整链路。

Q：目前完成到什么程度？

A：目前完成了 Logger、ThreadPool、Buffer、Config 和最小 Reactor Core。Reactor Core 已经能用 timerfd 验证 epoll 事件注册和回调分发。下一步是在它上面封装 TcpServer 和 Echo demo。

Q：你觉得项目里最核心的模块是什么？

A：当前阶段最核心的是 ThreadPool、Buffer 和 Reactor。ThreadPool 体现并发任务调度，Buffer 体现 TCP 字节流处理，Reactor 体现 Linux 服务端高并发 IO 模型。

### 7.2 线程池

Q：线程池怎么工作的？

A：提前创建固定数量 worker，任务通过 submit 放入队列，worker 没任务时阻塞在 condition_variable，有任务时被唤醒，取出任务执行。返回值通过 packaged_task 和 future 传回提交方。

Q：任务队列怎么保证线程安全？

A：所有对 `tasks_` 和 `stop_` 的访问都用同一个 mutex 保护，提交任务和 worker 取任务都在临界区内修改队列。

Q：为什么执行任务前要释放锁？

A：任务执行时间不可控，如果持锁执行，会阻塞其它线程提交任务和其它 worker 取任务，导致并发度下降甚至卡住。

### 7.3 Buffer

Q：为什么要有 Buffer？

A：因为 TCP 是字节流，没有消息边界。Buffer 用来暂存 socket 读到的数据，协议解析器从里面消费完整消息，可以处理半包和粘包。

Q：Buffer 内部怎么管理数据？

A：底层是 vector，维护 reader_index 和 writer_index。两者之间是可读数据，writer 后面是可写空间，reader 前面是已读可复用空间。

Q：空间不够怎么办？

A：先看尾部空间加前面已读空间是否够。如果够，就把可读数据移动到开头复用空间；如果还不够，就 resize 扩容。

### 7.4 Reactor

Q：你的 EventLoop 怎么工作？

A：EventLoop 循环调用 EpollPoller::poll，poll 内部调用 epoll_wait。拿到活跃 Channel 后，EventLoop 遍历它们并调用 handle_event，Channel 根据 revents 执行对应回调。

Q：Channel 是什么？

A：Channel 是 fd 的事件抽象。它保存 fd、关注的事件、实际发生的事件，以及读写关闭错误回调。它不拥有 fd，只负责事件分发。

Q：为什么要把 Socket、Channel、Poller、EventLoop 拆开？

A：Socket 管 fd 生命周期，Channel 管事件和回调，Poller 管 epoll，EventLoop 管事件循环。拆开后职责清楚，后续 TcpConnection 和 TcpServer 可以基于这些抽象组合。

---

## 8. 以后每写完代码要补充什么

每完成一个模块，都按这个模板追加：

```text
## 模块名

### 这个模块解决什么问题

### 核心类和关键成员

### 一次完整流程

### 面试官可能追问

### 我的回答模板

### 当前实现的不足
```

尤其要记录“当前实现的不足”。面试官问到缺陷时，不要硬撑，可以这样说：

> 我现在实现的是学习版，先抓主流程。比如当前还没有做完善的单元测试、压力测试、跨线程唤醒和连接生命周期管理。下一步我会在 TcpServer 里补这些。

这种回答比假装项目已经很完整要可信。

---

## 9. 每次面试前的 10 分钟复习顺序

1. 先背项目一句话介绍。
2. 画出项目主线：Logger / Config -> ThreadPool -> Buffer -> Reactor -> TcpServer -> HTTP。
3. 复述 ThreadPool 的 submit 到 worker 执行流程。
4. 复述 Buffer 的 reader_index / writer_index 模型。
5. 复述 Reactor 的 Channel / Poller / EventLoop 分工。
6. 准备一个“当前不足和下一步计划”的回答。

最低目标：面试官问项目时，先撑住前 10 分钟，把主动权拿回来。
