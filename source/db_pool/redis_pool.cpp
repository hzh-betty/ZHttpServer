#include "db_pool/redis_pool.h"
#include "log/http_logger.h"
#include "db_pool/db_exception.h"

namespace zhttp::zdb
{
    RedisConnectionPool::RedisConnectionPool()
    {
        ZHTTP_LOG_DEBUG("Creating Redis connection pool instance");
        check_thread_ = std::thread(&RedisConnectionPool::check_connections, this);
        check_thread_.detach();
        ZHTTP_LOG_DEBUG("Redis connection health check thread started");
    }

    RedisConnectionPool::~RedisConnectionPool()
    {
        ZHTTP_LOG_INFO("Destroying Redis connection pool");
        std::lock_guard<std::mutex> lock(mutex_);
        
        size_t connection_count = connections_.size();
        while (!connections_.empty())
        {
            connections_.pop();
        }
        initialized_ = false;
        
        ZHTTP_LOG_INFO("Redis connection pool destroyed, cleaned up {} connections", connection_count);
    }

    void RedisConnectionPool::init(const std::string &host, int port, const std::string &password, 
                                   int db, uint32_t pool_size, int timeout_ms)
    {
        ZHTTP_LOG_INFO("Initializing Redis connection pool with {} connections", pool_size);
        ZHTTP_LOG_DEBUG("Redis config: {}:{}, db={}", host, port, db);
        
        std::lock_guard<std::mutex> lockGuard(mutex_);

        if (initialized_) 
        {
            ZHTTP_LOG_WARN("Redis connection pool already initialized, skipping");
            return;
        }

        host_ = host;
        port_ = port;
        password_ = password;
        db_ = db;
        timeout_ms_ = timeout_ms;

        uint32_t created_connections = 0;
        for (uint32_t i = 0; i < pool_size; ++i)
        {
            try
            {
                ZHTTP_LOG_DEBUG("Creating Redis connection {}/{}", i + 1, pool_size);
                if (auto conn = create_connection())
                {
                    connections_.emplace(conn);
                    created_connections++;
                }
                else
                {
                    ZHTTP_LOG_ERROR("Failed to create Redis connection {}/{}", i + 1, pool_size);
                }
            }
            catch (const std::exception &e)
            {
                ZHTTP_LOG_ERROR("Exception creating Redis connection {}/{}: {}", i + 1, pool_size, e.what());
            }
        }

        initialized_ = true;
        ZHTTP_LOG_INFO("Redis connection pool initialized successfully with {}/{} connections", 
                      created_connections, pool_size);
    }

    std::shared_ptr<RedisConnection> RedisConnectionPool::get_connection()
    {
        ZHTTP_LOG_DEBUG("Requesting Redis connection from pool");
        
        std::shared_ptr<RedisConnection> conn;
        {
            std::unique_lock<std::mutex> lock(mutex_);

            if (!initialized_)
            {
                ZHTTP_LOG_ERROR("Redis connection pool not initialized");
                throw DBException("Redis connection pool not initialized");
            }

            ZHTTP_LOG_DEBUG("Waiting for available Redis connection, pool size: {}", connections_.size());
            cv_.wait(lock, [this]() { return !connections_.empty(); });

            conn = std::move(connections_.front());
            connections_.pop();
            
            ZHTTP_LOG_DEBUG("Redis connection acquired from pool, remaining connections: {}", connections_.size());
        }

        try
        {
            ZHTTP_LOG_DEBUG("Verifying Redis connection health");
            if (!conn->ping())
            {
                ZHTTP_LOG_WARN("Redis connection lost, attempting to reconnect...");
                conn->reconnect();
                ZHTTP_LOG_INFO("Redis connection reconnected successfully");
            }

            // 通过定制删除器，确保连接被正确地回收到连接池中
            return {conn.get(),
                    [this, conn](RedisConnection *)
                    {
                        ZHTTP_LOG_DEBUG("Returning Redis connection to pool");
                        try
                        {
                            conn->cleanup();
                            std::lock_guard<std::mutex> lock(mutex_);
                            connections_.emplace(conn);
                            cv_.notify_one();
                            ZHTTP_LOG_DEBUG("Redis connection returned to pool successfully, pool size: {}", connections_.size());
                        }
                        catch (const std::exception &e)
                        {
                            ZHTTP_LOG_ERROR("Error returning Redis connection to pool: {}", e.what());
                        }
                    }};
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Failed to get Redis connection: {}", e.what());
            {
                std::lock_guard<std::mutex> lock(mutex_);
                connections_.emplace(conn);
                cv_.notify_one();
                ZHTTP_LOG_DEBUG("Redis connection returned to pool due to error");
            }
            throw;
        }
    }

    std::shared_ptr<RedisConnection> RedisConnectionPool::create_connection()
    {
        ZHTTP_LOG_DEBUG("Creating new Redis connection");
        try
        {
            auto conn = std::make_shared<RedisConnection>(host_, port_, password_, db_, timeout_ms_);
            ZHTTP_LOG_DEBUG("Redis connection created successfully");
            return conn;
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Failed to create Redis connection: {}", e.what());
            return nullptr;
        }
    }

    void RedisConnectionPool::check_connections() const
    {
        ZHTTP_LOG_INFO("Redis connection health check thread started");
        
        while (true)
        {
            try
            {
                ZHTTP_LOG_DEBUG("Starting Redis connection health check cycle");
                
                std::vector<std::shared_ptr<RedisConnection>> connsToCheck;
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    if (connections_.empty())
                    {
                        ZHTTP_LOG_DEBUG("No Redis connections in pool to check");
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        continue;
                    }

                    // 复制连接进行检查，不移动原有连接
                    std::queue<std::shared_ptr<RedisConnection>> temp = connections_;
                    while (!temp.empty())
                    {
                        connsToCheck.emplace_back(temp.front());
                        temp.pop();
                    }
                    
                    ZHTTP_LOG_DEBUG("Checking {} Redis connections for health", connsToCheck.size());
                }

                // 在锁外检查连接健康状态
                size_t healthy_connections = 0;
                size_t reconnected_connections = 0;
                
                for (auto &conn : connsToCheck)
                {
                    try
                    {
                        if (!conn->ping())
                        {
                            ZHTTP_LOG_WARN("Unhealthy Redis connection detected, attempting reconnect");
                            conn->reconnect();
                            reconnected_connections++;
                            ZHTTP_LOG_INFO("Redis connection reconnected successfully");
                        }
                        healthy_connections++;
                    }
                    catch (const std::exception &e)
                    {
                        ZHTTP_LOG_ERROR("Failed to reconnect unhealthy Redis connection: {}", e.what());
                    }
                }
                
                ZHTTP_LOG_INFO("Redis health check completed: {}/{} connections healthy, {} reconnected", 
                              healthy_connections, connsToCheck.size(), reconnected_connections);

                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
            catch (const std::exception &e)
            {
                ZHTTP_LOG_ERROR("Error in Redis connection health check thread: {}", e.what());
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }

    size_t RedisConnectionPool::get_pool_size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t size = connections_.size();
        ZHTTP_LOG_DEBUG("Current Redis pool size: {}", size);
        return size;
    }

    bool RedisConnectionPool::is_initialized() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ZHTTP_LOG_DEBUG("Redis pool initialization status: {}", initialized_ ? "true" : "false");
        return initialized_;
    }

} // namespace zhttp::zdb
