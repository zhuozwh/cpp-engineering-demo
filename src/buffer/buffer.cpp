#include "buffer/buffer.h"

#include <algorithm>
#include <cassert>

namespace buffer {

Buffer::Buffer()
    : buffer_(kInitialSize),
      reader_index_(0),
      writer_index_(0) {
    // 初始状态：
    // readable_bytes() == 0
    // writable_bytes() == kInitialSize
}

// 可读数据区间是 [reader_index_, writer_index_)
// 所以可读字节数 = writer_index_ - reader_index_
std::size_t Buffer::readable_bytes() const {
    return writer_index_ - reader_index_;
}

// 可写空间区间是 [writer_index_, buffer_.size())
// 所以可写字节数 = buffer_.size() - writer_index_
std::size_t Buffer::writable_bytes() const {
    return buffer_.size() - writer_index_;
}

// reader_index_ 前面的区域是已经被读取过的空间。
// 这些空间后续可以被复用。
std::size_t Buffer::prependable_bytes() const {
    return reader_index_;
}

// 返回可读数据的起始地址。
// 注意：peek 只是“看一眼”，不会消费数据。
const char* Buffer::peek() const {
    return begin() + reader_index_;
}

void Buffer::retrieve(std::size_t len) {
    // 不能读取超过当前可读数据长度的内容。
    assert(len <= readable_bytes());

    if (len < readable_bytes()) {
        // 只消费一部分数据，reader_index_ 向后移动 len。
        reader_index_ += len;
    } else {
        // 如果消费掉全部可读数据，就直接重置索引。
        // 这样可以最大化复用整个 buffer_。
        retrieve_all();
    }
}

void Buffer::retrieve_all() {
    // 清空逻辑数据，但不释放底层 vector 空间。
    // 后续 append 可以继续复用这块内存。
    reader_index_ = 0;
    writer_index_ = 0;
}

std::string Buffer::retrieve_as_string(std::size_t len) {
    // 读取长度不能超过当前可读数据长度。
    assert(len <= readable_bytes());

    // 从 peek() 开始拷贝 len 个字节构造 string。
    std::string result(peek(), len);

    // 构造完 string 后，消费掉这 len 个字节。
    retrieve(len);

    return result;
}

std::string Buffer::retrieve_all_as_string() {
    // 把当前所有可读数据取出来。
    return retrieve_as_string(readable_bytes());
}

void Buffer::append(const char* data, std::size_t len) {
    // 写入前先保证有足够空间。
    ensure_writable_bytes(len);

    // 把外部数据复制到 writer_index_ 指向的位置。
    std::copy(data, data + len, begin() + writer_index_);

    // 写入完成后，writer_index_ 后移 len。
    writer_index_ += len;
}

void Buffer::append(const std::string& data) {
    // std::string 底层也是连续字符数据。
    append(data.data(), data.size());
}

char* Buffer::begin() {
    return buffer_.data();
}

const char* Buffer::begin() const {
    return buffer_.data();
}

void Buffer::ensure_writable_bytes(std::size_t len) {
    if (writable_bytes() < len) {
        // 当前尾部可写空间不够，需要腾空间或扩容。
        make_space(len);
    }
}

void Buffer::make_space(std::size_t len) {
    // writable_bytes(): writer_index_ 后面的空闲空间
    // prependable_bytes(): reader_index_ 前面已经读过、可复用的空间
    //
    // 如果两者加起来仍然不够 len，就只能扩容。
    if (writable_bytes() + prependable_bytes() < len) {
        // 扩容到：当前已写位置 + 还需要写入的 len。
        //
        // 注意这里不会丢失已有可读数据，
        // vector::resize 会保留原来的内容。
        buffer_.resize(writer_index_ + len);
    } else {
        // 如果总空间够，只是尾部空间不够，
        // 就把当前可读数据移动到 buffer_ 开头，
        // 复用前面已经读过的空间。
        std::size_t readable = readable_bytes();

        std::copy(begin() + reader_index_,
                  begin() + writer_index_,
                  begin());

        // 移动后，可读数据从 0 开始。
        reader_index_ = 0;

        // writer_index_ 指向可读数据末尾。
        writer_index_ = readable;
    }
}

}  // namespace buffer