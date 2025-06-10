#pragma once
#include "logger.hpp"
namespace zlog
{
    // 1. 提供获取指定日志器的全局接口--避免用户使用单例对象创建
    inline Logger::ptr getLogger(const std::string &name)
    {
        return LoggerManager::getInstance().getLogger(name);
    }
    inline Logger::ptr rootLogger()
    {
        return LoggerManager::getInstance().rootLogger();
    }

// 2. 通过宏函数对日志器的接口进行代理
#define ZLOG_DEBUG(fmt, ...) logImpl(zlog::LogLevel::value::DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define ZLOG_INFO(fmt, ...) logImpl(zlog::LogLevel::value::INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define ZLOG_WARN(fmt, ...) logImpl(zlog::LogLevel::value::WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define ZLOG_ERROR(fmt, ...) logImpl(zlog::LogLevel::value::ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define ZLOG_FATAL(fmt, ...) logImpl(zlog::LogLevel::value::FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

// 3. 提供宏函数，直接通过默认日志器打印
#define DEBUG(fmt, ...) zlog::rootLogger()->ZLOG_DEBUG(fmt, ##__VA_ARGS__)
#define INFO(fmt, ...) zlog::rootLogger()->ZLOG_INFO(fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) zlog::rootLogger()->ZLOG_WARN(fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) zlog::rootLogger()->ZLOG_ERROR(fmt, ##__VA_ARGS__)
#define FATAL(fmt, ...) zlog::rootLogger()->ZLOG_FATAL(fmt, ##__VA_ARGS__)

};