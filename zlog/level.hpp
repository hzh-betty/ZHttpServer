#pragma once
#include <iostream>
#include <string>

/*
    1. 定义枚举类：区分日志等级
    2. 提供转换接口：日志等级转换为字符串
*/

namespace zlog
{
    class LogLevel
    {
    public:
        enum class value
        {
            UNKNOWN = 0,
            DEBUG,
            INFO,
            WARNING,
            ERROR,
            FATAL,
            OFF,
        };
        
        static std::string toString(LogLevel::value level)
        {
            switch (level)
            {
            case LogLevel::value::DEBUG:
                return "DEBUG";
            case LogLevel::value::INFO:
                return "INFO";
            case LogLevel::value::WARNING:
                return "WARNING";
            case LogLevel::value::ERROR:
                return "ERROR";
            case LogLevel::value::FATAL:
                return "FATAL";
            }
            return "UNKNOWN";
        }
    };
};