#pragma once
#include "redis_connection.h"
#include <queue>
#include <condition_variable>
#include <thread>

namespace zhttp::zdb
{
    class RedisConnectionPool
    {
    public:
        // 禁止拷贝
        RedisConnectionPool(const RedisConnectionPool &) = delete;
        RedisConnectionPool &operator=(const RedisConnectionPool &) = delete;

        static RedisConnectionPool &get_instance()
        {
            static RedisConnectionPool instance;
            return instance;
        }

        // 初始化连接池
        void init(const std::string &host, int port = 6379, const std::string &password = "", 
                  int db = 0, uint32_t pool_size = 10, int timeout_ms = 5000);

        // 获取连接
        std::shared_ptr<RedisConnection> get_connection();

        // 获取连接池状态
        size_t get_pool_size() const;

        // 获取连接池是否已初始化
        bool is_initialized() const;

    private:
        RedisConnectionPool();
        ~RedisConnectionPool();

        // 创建连接
        std::shared_ptr<RedisConnection> create_connection();

        // 检查连接
        void check_connections() const;

    private:
        std::string host_;
        int port_{};
        std::string password_;
        int db_{};
        int timeout_ms_{};
        std::queue<std::shared_ptr<RedisConnection>> connections_;
        mutable std::mutex mutex_;
        std::condition_variable cv_;
        bool initialized_ = false;
        std::thread check_thread_;
    };
} // namespace zhttp::zdb
