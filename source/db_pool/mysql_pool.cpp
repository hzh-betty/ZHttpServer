#include "../../include/db_pool/mysql_pool.h"
#include "../../include/log/logger.h"

namespace zhttp::zdb
{
    // 初始化连接池
    void MysqlConnectionPool::init(const std::string &host, const std::string &user,
                                const std::string &password, const std::string &database, uint32_t pool_size)
    {
        ZHTTP_LOG_INFO("Initializing database connection pool with {} connections", pool_size);
        ZHTTP_LOG_DEBUG("Database config: {}@{}/{}", user, host, database);
        
        std::lock_guard<std::mutex> lockGuard(mutex_);

        // 确保只初始化一次
        if (initialized_) 
        {
            ZHTTP_LOG_WARN("Database connection pool already initialized, skipping");
            return;
        }

        host_ = host;
        user_ = user;
        password_ = password;
        database_ = database;

        uint32_t created_connections = 0;
        for (uint32_t i = 0; i < pool_size; ++i)
        {
            try
            {
                ZHTTP_LOG_DEBUG("Creating connection {}/{}", i + 1, pool_size);
                if (auto conn = create_connection())
                {
                    connections_.emplace(conn);
                    created_connections++;
                }
                else
                {
                    ZHTTP_LOG_ERROR("Failed to create connection {}/{}", i + 1, pool_size);
                }
            }
            catch (const std::exception &e)
            {
                ZHTTP_LOG_ERROR("Exception creating connection {}/{}: {}", i + 1, pool_size, e.what());
            }
        }

        initialized_ = true;
        ZHTTP_LOG_INFO("Database connection pool initialized successfully with {}/{} connections", 
                      created_connections, pool_size);
    }

    MysqlConnectionPool::MysqlConnectionPool()
    {
        ZHTTP_LOG_DEBUG("Creating database connection pool instance");
        check_thread_ = std::thread(&MysqlConnectionPool::check_connections, this);
        check_thread_.detach();
        ZHTTP_LOG_DEBUG("Connection health check thread started");
    }

    MysqlConnectionPool::~MysqlConnectionPool()
    {
        ZHTTP_LOG_INFO("Destroying database connection pool");
        std::lock_guard<std::mutex> lock(mutex_);
        
        size_t connection_count = connections_.size();
        while (!connections_.empty())
        {
            connections_.pop();
        }
        initialized_ = false;
        
        ZHTTP_LOG_INFO("Database connection pool destroyed, cleaned up {} connections", connection_count);
    }

    // 获取连接
    std::shared_ptr<MysqlConnection> MysqlConnectionPool::get_connection()
    {
        ZHTTP_LOG_DEBUG("Requesting database connection from pool");
        
        std::shared_ptr<MysqlConnection> conn;
        {
            std::unique_lock<std::mutex> lock(mutex_);

            if (!initialized_)
            {
                ZHTTP_LOG_ERROR("Connection pool not initialized");
                throw DBException("Connection pool not initialized");
            }

            ZHTTP_LOG_DEBUG("Waiting for available connection, pool size: {}", connections_.size());
            cv_.wait(lock, [this]() { return !connections_.empty(); });

            conn = std::move(connections_.front());
            connections_.pop();
            
            ZHTTP_LOG_DEBUG("Connection acquired from pool, remaining connections: {}", connections_.size());
        } // 释放锁

        try
        {
            // 在锁外检查连接
            ZHTTP_LOG_DEBUG("Verifying connection health");
            if (!conn->ping())
            {
                ZHTTP_LOG_WARN("Connection lost, attempting to reconnect...");
                conn->reconnect();
                ZHTTP_LOG_INFO("Connection reconnected successfully");
            }

            // 通过定制删除器，确保连接被正确地回收到连接池中
            return {conn.get(),
                    [this, conn](MysqlConnection *)
                    {
                        ZHTTP_LOG_DEBUG("Returning connection to pool");
                        try
                        {
                            conn->cleanup();
                            std::lock_guard<std::mutex> lock(mutex_);
                            connections_.emplace(conn);
                            cv_.notify_one();
                            ZHTTP_LOG_DEBUG("Connection returned to pool successfully, pool size: {}", connections_.size());
                        }
                        catch (const std::exception &e)
                        {
                            ZHTTP_LOG_ERROR("Error returning connection to pool: {}", e.what());
                        }
                    }};
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Failed to get connection: {}", e.what());
            {
                std::lock_guard<std::mutex> lock(mutex_);
                connections_.emplace(conn);
                cv_.notify_one();
                ZHTTP_LOG_DEBUG("Connection returned to pool due to error");
            }
            throw;
        }
    }

    // 创建连接
    std::shared_ptr<MysqlConnection> MysqlConnectionPool::create_connection()
    {
        ZHTTP_LOG_DEBUG("Creating new database connection");
        try
        {
            auto conn = std::make_shared<MysqlConnection>(host_, user_, password_, database_);
            ZHTTP_LOG_DEBUG("Database connection created successfully");
            return conn;
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Failed to create database connection: {}", e.what());
            return nullptr;
        }
    }

    // 修改检查连接的函数
    void MysqlConnectionPool::check_connections() const
    {
        ZHTTP_LOG_INFO("Database connection health check thread started");
        
        while (true)
        {
            try
            {
                ZHTTP_LOG_DEBUG("Starting connection health check cycle");
                
                std::vector<std::shared_ptr<MysqlConnection>> connsToCheck;
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    if (connections_.empty())
                    {
                        ZHTTP_LOG_DEBUG("No connections in pool to check");
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        continue;
                    }

                    auto temp = connections_;
                    while (!temp.empty())
                    {
                        connsToCheck.emplace_back(std::move(temp.front()));
                        temp.pop();
                    }
                    
                    ZHTTP_LOG_DEBUG("Checking {} connections for health", connsToCheck.size());
                }

                // 在锁外检查连接
                size_t healthy_connections = 0;
                size_t reconnected_connections = 0;
                
                for (auto &conn: connsToCheck)
                {
                    try
                    {
                        if (!conn->ping())
                        {
                            ZHTTP_LOG_WARN("Unhealthy connection detected, attempting reconnect");
                            conn->reconnect();
                            reconnected_connections++;
                            ZHTTP_LOG_INFO("Connection reconnected successfully");
                        }
                        healthy_connections++;
                    }
                    catch (const std::exception &e)
                    {
                        ZHTTP_LOG_ERROR("Failed to reconnect unhealthy connection: {}", e.what());
                    }
                }
                
                ZHTTP_LOG_INFO("Health check completed: {}/{} connections healthy, {} reconnected", 
                              healthy_connections, connsToCheck.size(), reconnected_connections);

                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
            catch (const std::exception &e)
            {
                ZHTTP_LOG_ERROR("Error in connection health check thread: {}", e.what());
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }

    // 获取连接池状态
    size_t MysqlConnectionPool::get_pool_size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t size = connections_.size();
        ZHTTP_LOG_DEBUG("Current pool size: {}", size);
        return size;
    }

    // 获取连接池是否已初始化
    bool MysqlConnectionPool::is_initialized() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ZHTTP_LOG_DEBUG("Pool initialization status: {}", initialized_ ? "true" : "false");
        return initialized_;
    }

} // namespace zhttp::zdb