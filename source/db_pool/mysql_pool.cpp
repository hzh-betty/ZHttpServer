#include "../../include/db_pool/mysql_pool.h"
#include "../../include/log/logger.h"

namespace zhttp::zdb
{
    // 初始化连接池
    void MysqlConnectionPool::init(const std::string &host, const std::string &user,
                                const std::string &password, const std::string &database, uint32_t pool_size)
    {
        ZHTTP_LOG_INFO("Initializing MySQL connection pool with {} connections", pool_size);
        ZHTTP_LOG_DEBUG("MySQL config: {}@{}/{}", user, host, database);
        
        std::lock_guard<std::mutex> lockGuard(mutex_);

        // 确保只初始化一次
        if (initialized_) 
        {
            ZHTTP_LOG_WARN("MySQL connection pool already initialized, skipping");
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
        ZHTTP_LOG_INFO("MySQL connection pool initialized successfully with {}/{} connections", 
                      created_connections, pool_size);
    }

    MysqlConnectionPool::MysqlConnectionPool()
    {
        ZHTTP_LOG_DEBUG("Creating MySQL connection pool instance");
        check_thread_ = std::thread(&MysqlConnectionPool::check_connections, this);
        check_thread_.detach();
        ZHTTP_LOG_DEBUG("MySQL connection health check thread started");
    }

    MysqlConnectionPool::~MysqlConnectionPool()
    {
        ZHTTP_LOG_INFO("Destroying MySQL connection pool");
        std::lock_guard<std::mutex> lock(mutex_);
        
        size_t connection_count = connections_.size();
        while (!connections_.empty())
        {
            connections_.pop();
        }
        initialized_ = false;
        
        ZHTTP_LOG_INFO("MySQL connection pool destroyed, cleaned up {} connections", connection_count);
    }

    // 获取连接
    std::shared_ptr<MysqlConnection> MysqlConnectionPool::get_connection()
    {
        ZHTTP_LOG_DEBUG("Requesting MySQL connection from pool");
        
        std::shared_ptr<MysqlConnection> conn;
        {
            std::unique_lock<std::mutex> lock(mutex_);

            if (!initialized_)
            {
                ZHTTP_LOG_ERROR("MySQL connection pool not initialized");
                throw DBException("Connection pool not initialized");
            }

            ZHTTP_LOG_DEBUG("Waiting for available MySQL connection, pool size: {}", connections_.size());
            cv_.wait(lock, [this]() { return !connections_.empty(); });

            conn = std::move(connections_.front());
            connections_.pop();
            
            ZHTTP_LOG_DEBUG("MySQL connection acquired from pool, remaining connections: {}", connections_.size());
        } // 释放锁

        try
        {
            // 在锁外检查连接
            ZHTTP_LOG_DEBUG("Verifying MySQL connection health");
            if (!conn->ping())
            {
                ZHTTP_LOG_WARN("MySQL connection lost, attempting to reconnect...");
                conn->reconnect();
                ZHTTP_LOG_INFO("MySQL connection reconnected successfully");
            }

            // 通过定制删除器，确保连接被正确地回收到连接池中
            return {conn.get(),
                    [this, conn](MysqlConnection *)
                    {
                        ZHTTP_LOG_DEBUG("Returning MySQL connection to pool");
                        try
                        {
                            conn->cleanup();
                            std::lock_guard<std::mutex> lock(mutex_);
                            connections_.emplace(conn);
                            cv_.notify_one();
                            ZHTTP_LOG_DEBUG("MySQL connection returned to pool successfully, pool size: {}", connections_.size());
                        }
                        catch (const std::exception &e)
                        {
                            ZHTTP_LOG_ERROR("Error returning MySQL connection to pool: {}", e.what());
                        }
                    }};
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Failed to get MySQL connection: {}", e.what());
            {
                std::lock_guard<std::mutex> lock(mutex_);
                connections_.emplace(conn);
                cv_.notify_one();
                ZHTTP_LOG_DEBUG("MySQL connection returned to pool due to error");
            }
            throw;
        }
    }

    // 创建连接
    std::shared_ptr<MysqlConnection> MysqlConnectionPool::create_connection()
    {
        ZHTTP_LOG_DEBUG("Creating new MySQL connection");
        try
        {
            auto conn = std::make_shared<MysqlConnection>(host_, user_, password_, database_);
            ZHTTP_LOG_DEBUG("MySQL connection created successfully");
            return conn;
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Failed to create MySQL connection: {}", e.what());
            return nullptr;
        }
    }

    // 修改检查连接的函数
    void MysqlConnectionPool::check_connections() const
    {
        ZHTTP_LOG_INFO("MySQL connection health check thread started");
        
        while (true)
        {
            try
            {
                ZHTTP_LOG_DEBUG("Starting MySQL connection health check cycle");
                
                std::vector<std::shared_ptr<MysqlConnection>> connsToCheck;
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    if (connections_.empty())
                    {
                        ZHTTP_LOG_DEBUG("No MySQL connections in pool to check");
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        continue;
                    }

                    auto temp = connections_;
                    while (!temp.empty())
                    {
                        connsToCheck.emplace_back(std::move(temp.front()));
                        temp.pop();
                    }
                    
                    ZHTTP_LOG_DEBUG("Checking {} MySQL connections for health", connsToCheck.size());
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
                            ZHTTP_LOG_WARN("Unhealthy MySQL connection detected, attempting reconnect");
                            conn->reconnect();
                            reconnected_connections++;
                            ZHTTP_LOG_INFO("MySQL connection reconnected successfully");
                        }
                        healthy_connections++;
                    }
                    catch (const std::exception &e)
                    {
                        ZHTTP_LOG_ERROR("Failed to reconnect unhealthy MySQL connection: {}", e.what());
                    }
                }
                
                ZHTTP_LOG_INFO("MySQL health check completed: {}/{} connections healthy, {} reconnected", 
                              healthy_connections, connsToCheck.size(), reconnected_connections);

                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
            catch (const std::exception &e)
            {
                ZHTTP_LOG_ERROR("Error in MySQL connection health check thread: {}", e.what());
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }

    // 获取连接池状态
    size_t MysqlConnectionPool::get_pool_size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t size = connections_.size();
        ZHTTP_LOG_DEBUG("Current MySQL pool size: {}", size);
        return size;
    }

    // 获取连接池是否已初始化
    bool MysqlConnectionPool::is_initialized() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ZHTTP_LOG_DEBUG("MySQL pool initialization status: {}", initialized_ ? "true" : "false");
        return initialized_;
    }

} // namespace zhttp::zdb