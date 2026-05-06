#include "buffer/buffer.h"

#include <iostream>
#include <string>

int main() {
    buffer::Buffer buf;

    // 向 Buffer 写入两段数据。
    // 此时 Buffer 内部可读内容是："hello, world"
    buf.append("hello", 5);
    buf.append(", world");

    std::cout << "readable bytes: "
              << buf.readable_bytes()
              << std::endl;

    // 读取前 5 个字节。
    // 返回 "hello"，同时 reader_index_ 向后移动 5。
    std::string first = buf.retrieve_as_string(5);
    std::cout << "first part: " << first << std::endl;

    // 此时 Buffer 中还剩 ", world" 没有被消费。
    // 继续追加 "!!!"，可读内容变成 ", world!!!"
    buf.append("!!!");

    // 读取剩余全部数据。
    // 返回 ", world!!!"，同时清空 Buffer 的逻辑内容。
    std::string rest = buf.retrieve_all_as_string();
    std::cout << "rest: " << rest << std::endl;

    // retrieve_all_as_string 后，可读字节数应该为 0。
    std::cout << "readable bytes after retrieve all: "
              << buf.readable_bytes()
              << std::endl;

    return 0;
}