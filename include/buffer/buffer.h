#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace buffer {

class Buffer {
public:
    // 初始缓冲区大小。
    // 后续如果写入数据超过这个大小，会自动扩容。
    static constexpr std::size_t kInitialSize = 1024;

    Buffer();

    // 当前可以被读取的数据长度：
    // [reader_index_, writer_index_)
    std::size_t readable_bytes() const;

    // 当前还能直接写入的空间长度：
    // [writer_index_, buffer_.size())
    std::size_t writable_bytes() const;

    // reader_index_ 前面的空间。
    // 这些空间通常是已经被 retrieve 掉的数据区域，可以在 make_space 中复用。
    std::size_t prependable_bytes() const;

    // 返回当前可读数据的起始地址。
    // 注意：这里只是查看数据，不会移动 reader_index_。
    const char* peek() const;

    // 消费 len 个字节。
    // 例如 HTTP Parser 已经解析了 len 字节，就可以调用 retrieve(len) 丢弃这部分数据。
    void retrieve(std::size_t len);

    // 消费全部可读数据，并重置 reader_index_ / writer_index_。
    void retrieve_all();

    // 读取 len 个字节并以 string 返回，同时移动 reader_index_。
    std::string retrieve_as_string(std::size_t len);

    // 读取全部可读数据并以 string 返回，同时清空 Buffer。
    std::string retrieve_all_as_string();

    // 向 Buffer 追加一段原始字符数据。
    void append(const char* data, std::size_t len);

    // 向 Buffer 追加 std::string。
    void append(const std::string& data);

private:
    // 返回底层 vector 的起始地址。
    // 非 const 版本用于写入数据。
    char* begin();

    // const 版本用于只读访问。
    const char* begin() const;

    // 确保至少有 len 个字节可写。
    // 如果空间不足，会调用 make_space。
    void ensure_writable_bytes(std::size_t len);

    // 为即将写入的 len 个字节腾出空间。
    // 优先复用前面已经读过的空间，不够再扩容。
    void make_space(std::size_t len);

private:
    // 底层连续存储空间。
    //
    // 逻辑结构：
    // | 已读区域 | 可读区域 | 可写区域 |
    // 0    reader_index_   writer_index_   buffer_.size()
    std::vector<char> buffer_;

    // 当前可读数据的起始位置。
    std::size_t reader_index_;

    // 当前可写位置，也就是可读数据的结束位置。
    std::size_t writer_index_;
};

}  // namespace buffer