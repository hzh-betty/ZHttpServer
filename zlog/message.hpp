#pragma once
#include "level.hpp"
#include "util.hpp"
#include <thread>
/*
    日志消息类的设计
        1. 日志输出时间
        2. 日志等级
        3. 源码文件名称与行号
        4. 线程ID
        5. 日志主体消息
        6. 日志器名称
*/
namespace zlog
{
    using threadId = std::thread::id;
    struct LogMessage
    {
        time_t curtime_;        // 日志输出时间
        LogLevel::value level_; // 日志等级
        const char *file_;      // 源码文件名称与行号
        size_t line_;
        threadId tid_;        // 线程ID
        const char *payload_; // 日志主体消息
        const char *loggerName_;  // 日志器名称

        LogMessage(LogLevel::value level,
                   const char *file, size_t line,
                   const char *payload, const char *loggerName)
            : curtime_(Date::getCurrentTime()), level_(level),
              file_(file), line_(line), tid_(std::this_thread::get_id()),
              payload_(payload), loggerName_(loggerName)
        {
        }
    };
};