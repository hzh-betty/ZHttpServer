#pragma once
#include "../../zlog/zlog.h"
namespace zhttp
{
    inline zlog::Logger::ptr http_logger;

    class Log
    {
    public:
        static void Init(zlog::LogLevel::value limitLevel = zlog::LogLevel::value::DEBUG)
        {
            std::unique_ptr<zlog::GlobalLoggerBuilder> builder(new zlog::GlobalLoggerBuilder());
            builder->buildLoggerName("http_logger");
            builder->buildLoggerLevel(limitLevel);
            builder->buildLoggerFormmater("[%c][%d][%f:%l][%p]  %m%n");
            builder->buildLoggerSink<zlog::StdOutSink>();
            http_logger = builder->build();
        }
    };

    // 便利的日志宏定义 - 使用fmt库格式
#define ZHTTP_LOG_DEBUG(fmt, ...) zhttp::http_logger->debug(fmt, ##__VA_ARGS__)
#define ZHTTP_LOG_INFO(fmt, ...)  zhttp::http_logger->info(fmt, ##__VA_ARGS__)
#define ZHTTP_LOG_WARN(fmt, ...)  zhttp::http_logger->warn(fmt, ##__VA_ARGS__)
#define ZHTTP_LOG_ERROR(fmt, ...)  zhttp::http_logger->error(fmt, ##__VA_ARGS__)
#define ZHTTP_LOG_FATAL(fmt, ...)  zhttp::http_logger->fatal(fmt, ##__VA_ARGS__)
};
