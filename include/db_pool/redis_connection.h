#pragma once

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <sw/redis++/redis++.h>
#include "db_exception.h"

namespace zhttp::zdb
{
    class RedisConnection
    {
    public:
        explicit RedisConnection(std::string host, int port = 6379, std::string password = "", 
                       int db = 0, int timeout_ms = 5000);

        ~RedisConnection();

        // 禁止拷贝与赋值
        RedisConnection(const RedisConnection &) = delete;
        RedisConnection &operator=(const RedisConnection &) = delete;

        // 判断连接是否有效
        [[nodiscard]] bool is_valid() const;

        // 重新连接
        void reconnect();

        // 清理
        void cleanup() const;

        // 检测连接是否有效
        bool ping() const;

        // Redis操作接口
        void set(const std::string &key, const std::string &value, std::chrono::seconds ttl = std::chrono::seconds(0)) const;
        std::string get(const std::string &key) const;
        bool exists(const std::string &key) const;
        bool del(const std::string &key) const;
        void hset(const std::string &key, const std::string &field, const std::string &value) const;
        std::string hget(const std::string &key, const std::string &field) const;
        std::unordered_map<std::string, std::string> hgetall(const std::string &key) const;
        void expire(const std::string &key, std::chrono::seconds ttl) const;
        std::vector<std::string> scan_keys(const std::string &pattern, size_t count = 100) const;

    private:
        // 辅助连接并配置
        void connect_helper();

    private:
        std::unique_ptr<sw::redis::Redis> redis_{}; // Redis连接
        std::string host_;
        int port_;
        std::string password_;
        int db_;
        int timeout_ms_;
        mutable std::mutex mutex_;
    };
} // namespace zhttp::zdb
