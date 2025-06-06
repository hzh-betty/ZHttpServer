#pragma once
#include "util.hpp"
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/format.h>
#include <fmt/os.h>
#include <string>
#include <chrono>
#include <fstream>

// 平台相关
#ifdef _WIN32
#include <Windows.h>
#endif

/*
    日志落地模块的实现
        1. 抽象落地基类
        2. 派生子类
        3. 使用工厂模式进行创建与表示的分离
*/
namespace zlog
{
    class LogSink
    {
    public:
        using ptr = std::shared_ptr<LogSink>;
        LogSink() {}
        virtual ~LogSink() {}
        virtual void log(const char *data, size_t len) = 0;
    };

    // 标准输出
    class StdOutSink : public LogSink
    {
    public:
        void log(const char *data, size_t len) override
        {
            fmt::print(stdout, "{:.{}}", data, len);
        }
    };

    class FileSink : public LogSink
    {
    public:
        FileSink(const std::string &pathname)
            : pathname_(pathname)
        {
            File::createDirectory(File::path(pathname_));
            ofs_.open(pathname_, std::ios::binary | std::ios::app);
        }

        void log(const char *data, size_t len) override
        {
            fmt::print(ofs_, "{:.{}}", data, len);
        }

    protected:
        std::string pathname_;
        std::ofstream ofs_;
    };

    class RollBySizeSink : public LogSink
    {
    public:
        RollBySizeSink(const std::string &basename, size_t maxSize)
            : basename_(basename),
              maxSize_(maxSize),
              curSize_(0),
              nameCount_(0)
        {
            // 1.创建日志文件所用的路径
            std::string pathname = createNewFile();
            File::createDirectory(File::path(pathname));

            // 2. 创建并打开日志文件
            ofs_.open(pathname, std::ios::binary | std::ios::app);
        }

        void log(const char *data, size_t len) override
        {
            if (curSize_ + len > maxSize_)
            {
                rollOver();
            }
            fmt::print(ofs_, "{:.{}}", data, len);
            curSize_ += len;
        }

    protected:
        // 创建新文件流的方法
        std::string createNewFile()
        {
            time_t t = Date::getCurrentTime();
            struct tm lt;
#ifdef _WIN32
            localtime_s(&lt, &t);
#else
            localtime_r(&t, &lt);
#endif
            std::string pathname = fmt::format("{}_{:%Y%m%d%H%M%S}-{}.log",
                                               basename_, lt, nameCount_++);

            return pathname;
        }

        void rollOver()
        {
            ofs_.close(); // 释放旧流资源
            std::string pathname = createNewFile();
            ofs_.open(pathname, std::ios::binary | std::ios::app);
            curSize_ = 0;
        }

        std::string basename_;
        std::ofstream ofs_;
        size_t maxSize_;
        size_t curSize_;
        size_t nameCount_;
    };

    // 修改工厂类支持移动语义
    class SinkFactory
    {
    public:
        template <typename SinkType, typename... Args>
        static LogSink::ptr create(Args &&...args)
        {
            // 使用完美转发构造对象
            return std::make_shared<SinkType>(std::forward<Args>(args)...);
        }
    };
};