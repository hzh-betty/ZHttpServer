#pragma once
#include <iostream>
#include <string>
#include <chrono>
#include<atomic>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace zlog
{
    /*
        1. 关于日期的常用接口
        2. 关于文件的常用接口
    */
    
    class Date
    {
    public:
        /*获取当前系统时间*/
        static time_t getCurrentTime()
        {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            return time;
        }
    };

    class File
    {
    public:
        /*判断文件是否存在*/
        static bool exists(const std::string &pathname)
        {
            struct stat st;
            if (stat(pathname.c_str(), &st) < 0)
            {
                return false;
            }
            return true;
        }

        /*查找文件所在路径*/
        static std::string path(const std::string &pathname)
        {
            size_t pos = pathname.find_last_of("/\\");
            if (pos == std::string::npos)
                return ".";
            return pathname.substr(0, pos + 1);
        }

        /*指定路径下创建文件夹*/
        static void createDirectory(const std::string &pathname)
        {
            // 循环创建目录(mkdir)
            // ./abc/bcd/efg
            size_t pos = 0;
            size_t index = 0;
            while (index < pathname.size())
            {
                pos = pathname.find_first_of("/\\", index);
                if (pos == std::string::npos)
                {
                    makeDir(pathname);
                    break;
                }
                std::string parentPath = pathname.substr(0, pos + 1);
                if (!exists(parentPath))
                {
                    makeDir(pathname);
                }
                index = pos + 1;
            }
        }

    private:
        static void makeDir(const std::string &pathname)
        {
#ifdef _WIN32
            _mkdir(pathname.c_str());
#else
            mkdir(pathname.c_str(), 0777);
#endif
        }
    };
};