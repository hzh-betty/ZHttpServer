#pragma once
#include "util.hpp"
#include <cassert>
#include <vector>
#include <algorithm>
namespace zlog
{
    static constexpr size_t DEFAULT_BUFFER_SIZE = 1024 * 1024 * 2;
    static constexpr size_t THRESHOLD_BUFFER_SIZE = 1024 * 1024 * 8;
    static constexpr size_t INCREMENT_BUFFER_SIZE = 1024 * 1024 * 1;
    class Buffer
    {
    public:
        Buffer()
            : buffer_(DEFAULT_BUFFER_SIZE), writerIdx_(0), readerIdx_(0)
        {
        }

        // 向缓冲区写入数据
        void push(const char *data, size_t len)
        {
            // 1.动态扩容
            ensureEnoughSize(len); // 动态空间增长，用于极限测试

            // 2.拷贝进缓冲区
            std::copy(data, data + len, &buffer_[writerIdx_]);

            // 3.将当前位置往后移动
            moveWriter(len);
        }

        // 返回可读数据的位置
        const char *begin()
        {
            return &buffer_[readerIdx_];
        }

        // 返回可写数据的长度
        size_t writeAbleSize()
        {
            // 为固定大小缓冲区提供!!!
            return (buffer_.size() - writerIdx_);
        }

        // 返回可读数据的长度
        size_t readAbleSize()
        {
            return writerIdx_ - readerIdx_;
        }

        // 移动读指针
        void moveReader(size_t len)
        {
            assert(len <= readAbleSize());
            readerIdx_ += len;
        }

        // 重置读写位置，初始化缓冲区
        void reset()
        {
            readerIdx_ = writerIdx_ = 0;
        }

        // 对Buffer实现交换操作
        void swap(Buffer &buffer)
        {
            buffer_.swap(buffer.buffer_);
            std::swap(readerIdx_, buffer.readerIdx_);
            std::swap(writerIdx_, buffer.writerIdx_);
        }

        // 判断缓冲区是否为空
        bool empty()
        {
            return readerIdx_ == writerIdx_;
        }

    private:
        // 对空间进行扩容
        void ensureEnoughSize(size_t len)
        {
            if (len <= writeAbleSize())
                return;
            size_t newSize = 0;
            // 1. 小于阈值翻倍增长
            if (buffer_.size() < THRESHOLD_BUFFER_SIZE)
            {
                newSize = buffer_.size() * 2 + len;
            }
            // 大于阈值增量增长
            else
            {
                newSize = buffer_.size() + INCREMENT_BUFFER_SIZE + len;
            }

            buffer_.resize(newSize);
        }
        
        // 移动写下标
        void moveWriter(size_t len)
        {
            assert(len <= writeAbleSize());
            writerIdx_ += len;
        }

    private:
        std::vector<char> buffer_; // 缓冲区
        size_t writerIdx_;         // 当前可写数据的下标
        size_t readerIdx_;         // 当前可读数据的下标
    };
};