# cpp_service_framework

轻量级 C++ 服务框架项目，用于展示 C++17、Linux 后端基础设施、并发编程、网络编程与工程化能力。

当前项目定位不是“大而全”的框架，而是围绕后端开发 / 软件开发实习与校招，逐步实现一个可运行、可测试、可讲设计、可写进简历的 C++ 服务端项目。

---

## 当前版本

**v0.4.0 Infra-Pack**

当前阶段完成基础设施模块：

- Logger：日志模块
- ThreadPool：固定线程数线程池，支持 `std::future` 返回值
- Buffer：可读 / 可写缓冲区，为后续网络层准备
- Config：轻量级 `key=value` 配置读取器

---

## 技术栈

- C++17
- CMake
- Linux
- STL
- `std::thread`
- `std::mutex`
- `std::condition_variable`
- `std::future`

---

## 项目结构

```text
include/
  logger/
  threadpool/
  buffer/
  config/

src/
  logger/
  threadpool/
  buffer/
  config/

examples/
  logger_test.cpp
  threadpool_demo.cpp
  buffer_demo.cpp
  config_demo.cpp

config/
  server.conf
```

---

## 构建方式

```bash
cmake -S . -B build
cmake --build build
```

---

## 运行示例

```bash
./build/logger_test
./build/threadpool_demo
./build/buffer_demo
./build/config_demo
```

---

## 模块说明

### Logger

日志模块用于统一项目中的日志输出。

当前支持：

- 日志级别
- 日志格式化
- `ConsoleSink`
- `FileSink`
- 基础线程安全输出

Logger 的设计目标是将日志消息、格式化逻辑和输出目标分离，为后续网络层、HTTP 层和业务 Demo 提供统一的日志能力。

---

### ThreadPool

固定线程数线程池，用于管理任务并发执行。

内部主要使用：

- `std::thread`
- `std::mutex`
- `std::condition_variable`
- 任务队列
- `std::future`
- `std::packaged_task`

当前支持：

- 创建固定数量 worker 线程
- 提交任务
- 使用 `std::future` 获取任务返回值
- 析构时优雅停止
- 已提交任务尽量执行完成

线程池基本流程：

```text
submit(task)
    ↓
任务进入队列
    ↓
condition_variable 唤醒 worker
    ↓
worker 从队列取出任务
    ↓
释放锁后执行任务
    ↓
future 获取返回值
```

---

### Buffer

Buffer 是为后续网络层准备的读写缓冲区。

它维护两个核心索引：

- `reader_index_`
- `writer_index_`

逻辑结构：

```text
| 已读区域 | 可读区域 | 可写区域 |
0      reader_index_   writer_index_   buffer_.size()
```

当前支持：

- `append` 写入数据
- `retrieve` 消费数据
- `retrieve_as_string` 读取并消费指定长度数据
- `retrieve_all_as_string` 读取并清空全部可读数据
- 查询 `readable_bytes`
- 查询 `writable_bytes`
- 空间复用
- 自动扩容

Buffer 后续会用于 `TcpConnection` 的输入缓冲区和输出缓冲区。

典型使用场景：

```text
socket read
    ↓
append 到 input buffer
    ↓
HTTP Parser 从 buffer 中解析
    ↓
解析完成后 retrieve 已处理数据
```

---

### Config

Config 是一个轻量级配置读取模块，用于读取 `key=value` 格式的配置文件。

示例配置：

```ini
server.port = 8080
server.thread_num = 4
log.level = INFO
log.file = logs/app.log
debug = true
```

当前支持：

- 跳过空行
- 跳过 `#` 注释
- 自动去除 key 和 value 两侧空白
- `get_string`
- `get_int`
- `get_bool`
- key 不存在时返回默认值
- 类型转换失败时返回默认值

示例代码：

```cpp
config::Config cfg;

if (cfg.load("config/server.conf")) {
    int port = cfg.get_int("server.port", 80);
    std::string level = cfg.get_string("log.level", "INFO");
    bool debug = cfg.get_bool("debug", false);
}
```

---

## 示例输出

### ThreadPool Demo

```text
task 0 is running in thread ...
task 1 is running in thread ...
task 2 is running in thread ...
task 3 is running in thread ...
task 0 finished
task 4 is running in thread ...
result = 0
result = 1
result = 4
...
```

### Buffer Demo

```text
readable bytes: 12
first part: hello
rest: , world!!!
readable bytes after retrieve all: 0
```

### Config Demo

```text
server.port = 8080
server.thread_num = 4
log.level = INFO
log.file = logs/app.log
debug = true
missing.value = default
```

---

## 版本路线

### 已完成

- v0.3.x：Logger 基础能力
- v0.4.0：Infra-Pack
  - Logger
  - ThreadPool
  - Buffer
  - Config

### 计划中

- v0.5.0：Reactor Core
  - Socket
  - Channel
  - EpollPoller
  - EventLoop

- v0.6.0：Tcp Echo Server
  - TcpConnection
  - TcpServer
  - Echo demo

- v0.7.0：HTTP MVP
  - HTTPRequest
  - HTTPResponse
  - HTTPParser
  - Router
  - HTTPServer

- v0.8.0：Redis Demo
  - RedisClient
  - 短链服务或文本分享服务 Demo

- v0.9.0：Engineering
  - 单元测试
  - benchmark
  - 压测脚本
  - CI
  - 文档完善

- v1.0.0：Job-Ready
  - 稳定 Demo
  - 完整 README
  - Release Notes
  - 简历描述
  - 面试讲解材料

---

## 当前阶段说明

v0.4.0 的目标是完成后续网络层所需的基础设施。

当前阶段暂不实现复杂网络逻辑，而是先保证基础模块可以独立运行、独立验证、独立讲清楚。

后续进入 v0.5.0 后，将开始实现基于 Linux `epoll` 的 Reactor 网络核心。

---

## 求职展示重点

这个项目当前可以体现：

- C++17 基础能力
- CMake 项目组织
- 模块化设计
- 日志系统设计
- 线程池与并发控制
- `mutex` / `condition_variable` 使用
- `future` / `packaged_task` 使用
- 网络缓冲区设计
- 配置文件解析
- 面向后端服务框架的逐步演进能力

---

## 下一步

下一阶段目标：

**v0.5.0 Reactor Core**

重点实现：

- Socket 封装
- Channel 事件抽象
- EpollPoller
- EventLoop
- 最小事件循环 demo
