/*
    完成日志器模块
        1. 同步日志器
        2. 异步日志器
*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "util.hpp"
#include "level.hpp"
#include "format.hpp"
#include "message.hpp"
#include "sink.hpp"
#include "looper.hpp"
#include "objectpool.hpp"
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <fmt/format.h>

namespace zlog
{

    class Logger
    {
    public:
        using ptr = std::shared_ptr<Logger>;
        Logger(const char *loggerName, LogLevel::value limitLevel,
               Formmatter::ptr &formmatter,
               std::vector<LogSink::ptr> &sinks) : cnt_(1),
                                                   loggerName_(loggerName),
                                                   limitLevel_(limitLevel), formmatter_(formmatter), sinks_(sinks.begin(), sinks.end())
        {
        }

        const std::string getName() const
        {
            return std::string(loggerName_);
        }

        template <typename Level, typename... Args>
        void logImpl(Level level, const char *file, size_t line, const char *fmt, Args &&...args)
        {
            logImplHelper(level, file, line, fmt, std::forward<Args>(args)...);
        }

    protected:
        template <typename... Args>
        void logImplHelper(LogLevel::value level, const char *file, size_t line, const char *fmt, Args &&...args)
        {
            if (level < limitLevel_)
                return;

            // 线程局部缓冲区，预分配内存并复用
            thread_local fmt::memory_buffer fmtBuffer;
            fmtBuffer.clear(); // 清空旧数据

            // 格式化到缓冲区
            fmt::vformat_to(std::back_inserter(fmtBuffer), fmt, fmt::make_format_args((args)...));

            // 添加终止符（如需要C风格字符串）
            fmtBuffer.push_back('\0');

            // 使用缓冲区内容（例如输出或转换为字符串）
            serialize(level, file, line, fmtBuffer.data());
        }

        void serialize(LogLevel::value level, const char *file, size_t line, const char *data)
        {
            // 3. 构建LogMessage对象
            size_t threId = (line + cnt_) % DEFAULT_POOL_NUM;
            cnt_++;
            LogMessage *msg = MessagePool::getInstance().alloc(threId, level, file, line, data, loggerName_);

            // 4.格式化
            thread_local fmt::memory_buffer buffer;
            buffer.clear();
            formmatter_->format(buffer, *msg); // 直接操作缓冲区

            // 5. 日志落地
            MessagePool::getInstance().dealloc(msg, threId);
            log(buffer.data(), buffer.size());
        }
        virtual void log(const char *data, size_t len) = 0;

    protected:
        std::atomic<int> cnt_;
        std::mutex mutex_;
        const char *loggerName_;
        std::atomic<LogLevel::value> limitLevel_;
        Formmatter::ptr formmatter_;
        std::vector<LogSink::ptr> sinks_;
    };

    /*同步日志器负责通过日志落地模块进行落地*/
    class SyncLogger : public Logger
    {
    public:
        SyncLogger(const char *loggerName, LogLevel::value limitLevel,
                   Formmatter::ptr &formmatter,
                   std::vector<LogSink::ptr> &sinks) : Logger(loggerName, limitLevel, formmatter, sinks)
        {
        }

    protected:
        void log(const char *data, size_t len) override
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (sinks_.empty())
                return;
            for (auto &sink : sinks_)
            {
                sink->log(data, len);
            }
        }
    };

    /*异步日志器*/
    class AsyncLogger : public Logger
    {
    public:
        AsyncLogger(const char *loggerName, LogLevel::value limitLevel,
                    Formmatter::ptr &formmatter,
                    std::vector<LogSink::ptr> &sinks, AsyncType looperType,
                    std::chrono::milliseconds milliseco) : Logger(loggerName, limitLevel, formmatter, sinks),
                                                           looper_(std::make_shared<AsyncLooper>(std::bind(&AsyncLogger::reLog,
                                                                                                           this, std::placeholders::_1),
                                                                                                 looperType, milliseco))

        {
        }

    protected:
        // 将数据写入到缓冲区
        void log(const char *data, size_t len) override
        {
            looper_->push(data, len);
        }

        // 设计一个实际落地函数，将数据从缓冲区中落地
        void reLog(Buffer &buffer)
        {
            if (sinks_.empty())
                return;
            for (auto &sink : sinks_)
            {
                sink->log(buffer.begin(), buffer.readAbleSize());
            }
        }

    protected:
        AsyncLooper::ptr looper_;
    };

    /*使用建造者模式，降低使用户使用成本*/
    enum class LoggerType
    {
        LOGGER_SYNC,
        LOGGER_ASYNC
    };

    class LoggerBuilder
    {
    public:
        LoggerBuilder()
            : loggerType_(LoggerType::LOGGER_SYNC), limitLevel_(LogLevel::value::DEBUG),
              looperType_(AsyncType::ASYNC_SAFE), milliseco_(std::chrono::milliseconds(3000))
        {
        }
        void buildLoggerType(LoggerType loggerType)
        {
            loggerType_ = loggerType;
        }

        void buildEnalleUnSafe()
        {
            looperType_ = AsyncType::ASYNC_UNSAFE;
        }

        void buildLoggerName(const char *loggerName)
        {
            loggerName_ = loggerName;
        }

        void buildLoggerLevel(LogLevel::value limitLevel)
        {
            limitLevel_ = limitLevel;
        }

        void buildWaitTime(std::chrono::milliseconds milliseco)
        {
            milliseco_ = milliseco;
        }

        void buildLoggerFormmater(const std::string &pattern)
        {
            formmatter_ = std::make_shared<Formmatter>(pattern);
        }

        template <typename SinkType, typename... Args>
        void buildLoggerSink(Args &&...args)
        {
            LogSink::ptr psink = SinkFactory::create<SinkType>(std::forward<Args>(args)...);
            sinks_.push_back(psink);
        }
        virtual Logger::ptr build() = 0;

    protected:
        LoggerType loggerType_;
        const char *loggerName_ = nullptr;
        LogLevel::value limitLevel_;
        Formmatter::ptr formmatter_;
        std::vector<LogSink::ptr> sinks_;
        AsyncType looperType_;
        std::chrono::milliseconds milliseco_;
    };

    /* 局部日志器建造者 */
    class LocalLoggerBuilder : public LoggerBuilder
    {
    public:
        Logger::ptr build() override
        {
            // 必须有日志器名称
            if (loggerName_ == nullptr)
            {
                return Logger::ptr();
            }

            if (formmatter_.get() == nullptr)
            {
                formmatter_ = std::make_shared<Formmatter>();
            }

            if (sinks_.empty())
            {
                buildLoggerSink<StdOutSink>();
            }

            if (loggerType_ == LoggerType::LOGGER_ASYNC)
            {
                return std::make_shared<AsyncLogger>(loggerName_, limitLevel_, formmatter_, sinks_, looperType_, milliseco_);
            }
            return std::make_shared<SyncLogger>(loggerName_, limitLevel_, formmatter_, sinks_);
        }
    };

    /*全局日志管理器，负责将每个日志器管理并实现全局访问*/
    class LoggerManager
    {
    public:
        static LoggerManager &getInstance()
        {
            static LoggerManager eton;
            return eton;
        }
        void addLogger(Logger::ptr &logger)
        {
            if (hasLogger(logger->getName()))
                return;
            std::unique_lock<std::mutex> lock(mutex_);
            loggers_.insert({logger->getName(), logger});
        }

        bool hasLogger(const std::string &name)
        {
            std::unique_lock<std::mutex>(mutex_);
            auto iter = loggers_.find(name);
            if (iter == loggers_.end())
            {
                return false;
            }
            return true;
        }
        Logger::ptr getLogger(const std::string &name)
        {
            std::unique_lock<std::mutex>(mutex_);
            auto iter = loggers_.find(name);
            if (iter == loggers_.end())
            {
                return Logger::ptr(); // 匿名对象
            }
            return iter->second;
        }

        Logger::ptr rootLogger()
        {
            return rootLogger_;
        }

    private:
        LoggerManager()
        {
            std::unique_ptr<zlog::LocalLoggerBuilder> builder(new zlog::LocalLoggerBuilder());
            builder->buildLoggerName("root");
            rootLogger_ = builder->build();
            loggers_.insert({"root", rootLogger_});
        }

    private:
        std::mutex mutex_;
        Logger::ptr rootLogger_; // 默认日志器
        std::unordered_map<std::string, Logger::ptr> loggers_;
    };

    /* 全局日志器建造者 */
    class GlobalLoggerBuilder : public LoggerBuilder
    {
    public:
        Logger::ptr build() override
        {
            // 必须要有日志器名
            if (loggerName_ == nullptr)
            {
                return Logger::ptr();
            }

            if (formmatter_.get() == nullptr)
            {
                formmatter_ = std::make_shared<Formmatter>();
            }

            if (sinks_.empty())
            {
                buildLoggerSink<StdOutSink>();
            }

            Logger::ptr logger;
            if (loggerType_ == LoggerType::LOGGER_ASYNC)
            {
                logger = std::make_shared<AsyncLogger>(loggerName_, limitLevel_, formmatter_, sinks_, looperType_, milliseco_);
            }
            else
            {
                logger = std::make_shared<SyncLogger>(loggerName_, limitLevel_, formmatter_, sinks_);
            }
            LoggerManager::getInstance().addLogger(logger);
            return logger;
        }
    };

};
