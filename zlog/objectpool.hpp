#pragma once
#include "util.hpp"
#include "message.hpp"
#include <mutex>
#include <condition_variable>
#include <vector>
#include<memory>
#ifdef _WIN32
#include <windows.h>
#undef ERROR
#undef max        // 屏蔽 max 宏
#else
#include <sys/mman.h> // Linux mmap头文件
#endif

/*
    解决message频繁创建释放->创建一个对象池对每个message对象进行复用    
*/
namespace zlog
{
    // 按页数申请内存（1页=4KB）
    inline static void *SystemAlloc(size_t kpage)
    {
        void *ptr = nullptr;

#ifdef _WIN32
        // Windows使用VirtualAlloc，每页4KB（左移12位）
        ptr = VirtualAlloc(0, kpage << 12, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
        // Linux使用mmap分配匿名内存，参数说明：
        // - MAP_ANONYMOUS: 不关联文件的内存映射
        // - MAP_PRIVATE: 私有映射（不可共享）
        // - PROT_READ | PROT_WRITE: 可读可写权限
        ptr = mmap(0, kpage * 4096, PROT_READ | PROT_WRITE,
                   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (ptr == MAP_FAILED)
        {
            ptr = nullptr; // 映射失败
        }
#endif

        if (!ptr)
        {
            throw std::bad_alloc();
        }
        return ptr;
    }

    // 释放内存
    inline static void SystemFree(void *ptr, size_t kpage)
    {
#ifdef _WIN32
        VirtualFree(ptr, 0, MEM_RELEASE);
#else
        // Linux通过munmap释放，需传递内存块大小
        munmap(ptr, kpage * 4096);
#endif
    }

    template <typename T, size_t PAGE>
    class FreeList
    {
    public:
        // 禁用拷贝
        FreeList(const FreeList&) = delete;
        FreeList& operator=(const FreeList&) = delete;
        
        // 禁用移动
        FreeList(FreeList&&) = delete;
        FreeList& operator=(FreeList&&) = delete;
        FreeList()
        {
            remainBytes_ = PAGE << 12; // PAGE*4kb
            start_ = memory_ = reinterpret_cast<char *>(SystemAlloc(PAGE));
            if (memory_ == nullptr)
            {
                throw std::bad_alloc();
            }
        }

        T *alloc()
        {
            T *obj = nullptr;
            // 1. 保证对象能存储的下
            size_t objSize = sizeof(T) < sizeof(void *) ? sizeof(void *) : sizeof(T);

            // 2. 复用自由链表
            std::unique_lock<std::mutex> lock(mutex_);
            condEmpty_.wait(lock, [&]()
                            { return remainBytes_ >= objSize || freeList_ != nullptr; });

            if (freeList_ != nullptr)
            {
                obj = reinterpret_cast<T *>(freeList_);
                freeList_ = nextObj(freeList_);
            }
            else
            {
                // 3. 从大块内存中切出objSize字节的内存
                obj = reinterpret_cast<T *>(memory_);
                memory_ += objSize;
                remainBytes_ -= objSize;
            }

            return obj;
        }

        void dealloc(T *obj)
        {
            // 1. 析构函数清理对象
            obj->~T();

            // 2. 头插到自由链表
            std::unique_lock<std::mutex> lock(mutex_);
            nextObj(obj) = freeList_;
            freeList_ = obj;
            condEmpty_.notify_one();
        }

        ~FreeList()
        {
            SystemFree(start_, PAGE);
        }

    private:
        void *&nextObj(void *ptr)
        {
            return *(reinterpret_cast<void **>(ptr));
        }

    private:
        char *start_ = nullptr;             // 指向大块内存的起始指针
        char *memory_ = nullptr;            // 指向大块内存的指针
        size_t remainBytes_ = 0;            // 剩余字节数
        void *freeList_;                    // 自由链表头指针
        std::mutex mutex_;                  // 互斥锁
        std::condition_variable condEmpty_; // 条件变量
    };

    static constexpr size_t DEFAULT_PAGE = 1;     // 申请页数
    static constexpr size_t DEFAULT_POOL_NUM = 8; // 桶的个数

    class MessagePool
    {
        using obj = FreeList<LogMessage, DEFAULT_PAGE>;
    public:
        static MessagePool &getInstance()
        {
            static MessagePool pool;
            return pool;
        }

        LogMessage *alloc(size_t id, LogLevel::value level,
            const char* file, size_t line,
            const char* payload, const char*loggername)
        {
            LogMessage *ptr = messPool_[id]->alloc();
            // 定位new初始化
            new (ptr) LogMessage(level, file, line, payload, loggername);
            return ptr;
        }

        void dealloc(LogMessage *ptr, size_t id)
        {
            messPool_[id]->dealloc(ptr);
        }

    private:
        MessagePool()
        {
            messPool_.reserve(DEFAULT_POOL_NUM); // 预分配内存但不构造对象
            for (size_t i = 0; i < DEFAULT_POOL_NUM; ++i)
            {
                std::unique_ptr<obj> ptr(new obj());
                messPool_.emplace_back(std::move(ptr)); // 直接构造元素，避免移动操作
            }
        }

    private:
        std::vector<std::unique_ptr<obj>> messPool_;
    };
};