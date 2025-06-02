#include "../../include/db_pool/db_pool.h"

namespace zhttp::zdb
{
    // 初始化连接池
    void DbConnectionPool::init(const std::string &host, const std::string &user,
                                const std::string &password, const std::string &database, uint32_t pool_size)
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);

        // 确保只初始化一次
        if (initialized_) return;

        host_ = host;
        user_ = user;
        password_ = password;
        database_ = database;

        for (uint32_t i = 0; i < pool_size; ++i)
        {
            connections_.emplace(create_connection());
        }

        initialized_ = true;
        LOG_INFO << "Database connection pool initialized with " << pool_size << " connections";
    }

    DbConnectionPool::DbConnectionPool()
    {
        check_thread_ = std::thread(&DbConnectionPool::check_connections, this);
        check_thread_.detach();
    }

    DbConnectionPool::~DbConnectionPool()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connections_.empty())
        {
            connections_.pop();
        }
        initialized_ = false;
        LOG_INFO << "Database connection pool destroyed";
    }

    // 获取连接
    std::shared_ptr<DbConnection> DbConnectionPool::get_connection()
    {
        std::shared_ptr<DbConnection> conn;
        {
            std::unique_lock<std::mutex> lock(mutex_);

            if (!initialized_)
            {
                throw DBException("Connection pool not initialized");
            }

            cv_.wait(lock, [this]() { return !connections_.empty(); });

            conn = std::move(connections_.front());
            connections_.pop();
        } // 释放锁

        try
        {
            // 在锁外检查连接
            if (!conn->ping())
            {
                LOG_WARN << "Connection lost, attempting to reconnect...";
                conn->reconnect();
            }

            // 通过定制删除器，确保连接被正确地回收到连接池中
            return {conn.get(),
                    [this, conn](DbConnection *)
                    {
                        conn->cleanup();
                        std::lock_guard<std::mutex> lock(mutex_);
                        connections_.emplace(conn);
                        cv_.notify_one();
                    }};
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Failed to get connection: " << e.what();
            {
                std::lock_guard<std::mutex> lock(mutex_);
                connections_.emplace(conn);
                cv_.notify_one();
            }
            throw;
        }
    }

    // 创建连接
    std::shared_ptr<DbConnection> DbConnectionPool::create_connection()
    {
        try
        {
            return std::make_shared<DbConnection>(host_, user_, password_, database_);
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Failed to create connection: " << e.what();
            return nullptr;
        }
    }


    // 修改检查连接的函数
    void DbConnectionPool::check_connections() const
    {
        while (true)
        {
            try
            {
                std::vector<std::shared_ptr<DbConnection>> connsToCheck;
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    if (connections_.empty())
                    {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        continue;
                    }

                    auto temp = connections_;
                    while (!temp.empty())
                    {
                        connsToCheck.emplace_back(std::move(temp.front()));
                        temp.pop();
                    }
                }

                // 在锁外检查连接
                for (auto &conn: connsToCheck)
                {
                    if (!conn->ping())
                    {
                        try
                        {
                            conn->reconnect();
                        }
                        catch (const std::exception &e)
                        {
                            LOG_ERROR << "Failed to reconnect: " << e.what();
                        }
                    }
                }

                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Error in check thread: " << e.what();
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }

} // namespace zhttp::zdb