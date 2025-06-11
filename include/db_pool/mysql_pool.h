#pragma once
#include "mysql_connection.h"
#include <queue>
#include <condition_variable>
#include <thread>

namespace zhttp::zdb
{
    class MysqlConnectionPool
    {
    public:
        // 禁止拷贝
        MysqlConnectionPool(const MysqlConnectionPool &) = delete;

        MysqlConnectionPool &operator=(const MysqlConnectionPool &) = delete;

        static MysqlConnectionPool &get_instance()
        {
            static MysqlConnectionPool instance;
            return instance;
        }

        // 初始化连接池
        void init(const std::string &host, const std::string &user,
                  const std::string &password, const std::string &database, uint32_t pool_size = 10);

        // 获取连接
        std::shared_ptr<MysqlConnection> get_connection();

        // 获取连接池状态
        size_t get_pool_size() const;

        // 获取连接池是否已初始化
        bool is_initialized() const;

    private:
        // 构造函数
        MysqlConnectionPool();

        ~MysqlConnectionPool();

        // 获取连接
        std::shared_ptr<MysqlConnection> create_connection();

        //  检查连接
        void check_connections() const;

    private:
        std::string host_;
        std::string user_;
        std::string password_;
        std::string database_;
        std::queue<std::shared_ptr<MysqlConnection> > connections_;
        mutable std::mutex mutex_;
        std::condition_variable cv_;
        bool initialized_ = false;
        std::thread check_thread_; // 添加检查线程
    };
} // namespace zhttp::zdb
