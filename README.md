## Logger Module

当前已完成 Logger v0.3.1，具备以下能力：

- 支持日志级别过滤：DEBUG / INFO / WARN / ERROR
- 支持多 sink 输出，可同时写入控制台和文件
- 引入 `LogMessage` 结构，统一承载日志上下文
- 引入 `LogFormatter`，实现日志格式化与输出职责分离
- 支持多线程环境下的基本安全输出
- 日志内容包含时间、级别、线程 id、文件名、行号、函数名和消息内容

### Current Design

日志模块当前采用如下职责划分：

- `Logger`：日志协调者，负责级别过滤、组织日志流程、分发到多个 sink
- `LogMessage`：日志消息实体，保存一次日志的完整上下文
- `LogFormatter`：负责将 `LogMessage` 格式化为字符串
- `LogSink`：输出抽象接口
- `ConsoleSink`：输出到控制台
- `FileSink`：输出到文件

日志调用流程如下：

```text
LOG_INFO("message")
    ↓
Logger::log(...)
    ↓
组装 LogMessage
    ↓
LogFormatter::format(...)
    ↓
写入一个或多个 sink
```

### Example Output

```text
[2026-04-20 21:49:44][INFO][tid:12345][logger_test.cpp:11][worker] worker 1 log message 10
```

### Demo

当前可通过 `logger_test` 运行日志模块示例：

```bash
cmake -S . -B build
cmake --build build
./build/logger_test
```

运行后可以看到：

- 控制台日志输出
- 文件日志输出（如 `app.log`）

### Next Step

Logger 下一阶段可继续增强：

- 自定义格式模板
- 异步日志
- 按级别拆分输出文件
- 更完善的测试与 benchmark