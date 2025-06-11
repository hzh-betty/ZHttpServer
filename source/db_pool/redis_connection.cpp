#include "../../include/db_pool/redis_connection.h"
#include "../../include/log/logger.h"
#include "../../include/db_pool/db_exception.h"

namespace zhttp::zdb
{
    RedisConnection::RedisConnection(std::string host, int port, std::string password, 
                                   int db, int timeout_ms)
        : host_(std::move(host)), port_(port), password_(std::move(password)), 
          db_(db), timeout_ms_(timeout_ms)
    {
        ZHTTP_LOG_DEBUG("Creating Redis connection to {}:{}/{}", host_, port_, db_);
        try
        {
            std::lock_guard<std::mutex> lockGuard(mutex_);
            connect_helper();
            ZHTTP_LOG_DEBUG("Redis connection created successfully");
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Failed to create Redis connection: {}", e.what());
            throw DBException(e.what());
        }
    }

    RedisConnection::~RedisConnection()
    {
        ZHTTP_LOG_DEBUG("Destroying Redis connection");
        cleanup();
        ZHTTP_LOG_INFO("Redis connection closed");
    }

    bool RedisConnection::ping() const
    {
        ZHTTP_LOG_DEBUG("Pinging Redis connection");
        try
        {
            std::lock_guard<std::mutex> lockGuard(mutex_);
            if (!redis_) 
            {
                ZHTTP_LOG_WARN("Redis connection is null, ping failed");
                return false;
            }

            redis_->ping();
            ZHTTP_LOG_DEBUG("Redis ping successful");
            return true;
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Redis ping failed: {}", e.what());
            return false;
        }
    }

    bool RedisConnection::is_valid() const
    {
        ZHTTP_LOG_DEBUG("Validating Redis connection");
        return ping();
    }

    void RedisConnection::reconnect()
    {
        ZHTTP_LOG_INFO("Attempting to reconnect to Redis {}:{}/{}", host_, port_, db_);
        try
        {
            std::lock_guard<std::mutex> lockGuard(mutex_);

            redis_.reset();
            ZHTTP_LOG_DEBUG("Old Redis connection released");

            connect_helper();
            ZHTTP_LOG_INFO("Redis reconnection successful");
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Redis reconnect failed: {}", e.what());
            throw DBException(e.what());
        }
    }

    void RedisConnection::cleanup() const
    {
        ZHTTP_LOG_DEBUG("Cleaning up Redis connection");
        try
        {
            std::lock_guard<std::mutex> lockGuard(mutex_);
            if (redis_)
            {
                // Redis连接清理相对简单，主要是释放资源
                ZHTTP_LOG_DEBUG("Redis connection cleanup completed");
            }
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_WARN("Error cleaning up Redis connection: {}", e.what());
        }
    }

    void RedisConnection::connect_helper()
    {
        ZHTTP_LOG_DEBUG("Establishing Redis connection");
        
        try
        {
            sw::redis::ConnectionOptions opts;
            opts.host = host_;
            opts.port = port_;
            opts.db = db_;
            opts.socket_timeout = std::chrono::milliseconds(timeout_ms_);
            opts.connect_timeout = std::chrono::milliseconds(timeout_ms_);
            
            if (!password_.empty())
            {
                opts.password = password_;
            }

            sw::redis::ConnectionPoolOptions pool_opts;
            pool_opts.size = 1; // 每个连接对象内部只维护一个连接

            redis_ = std::make_unique<sw::redis::Redis>(opts, pool_opts);
            ZHTTP_LOG_DEBUG("Redis connection established to {}:{}", host_, port_);

            // 测试连接
            redis_->ping();
            ZHTTP_LOG_INFO("Redis connection fully established and configured");
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Failed to create Redis connection: {}", e.what());
            throw DBException("Failed to create Redis connection: " + std::string(e.what()));
        }
    }

    void RedisConnection::set(const std::string &key, const std::string &value, const std::chrono::seconds ttl) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            if (ttl.count() > 0)
            {
                redis_->setex(key, ttl, value);
            }
            else
            {
                redis_->set(key, value);
            }
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Redis SET failed for key {}: {}", key, e.what());
            throw DBException(e.what());
        }
    }

    std::string RedisConnection::get(const std::string &key) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            auto value = redis_->get(key);
            return value ? *value : "";
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Redis GET failed for key {}: {}", key, e.what());
            throw DBException(e.what());
        }
    }

    bool RedisConnection::exists(const std::string &key) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            return redis_->exists(key) > 0;
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Redis EXISTS failed for key {}: {}", key, e.what());
            throw DBException(e.what());
        }
    }

    bool RedisConnection::del(const std::string &key) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            return redis_->del(key) > 0;
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Redis DEL failed for key {}: {}", key, e.what());
            throw DBException(e.what());
        }
    }

    void RedisConnection::hset(const std::string &key, const std::string &field, const std::string &value) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            redis_->hset(key, field, value);
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Redis HSET failed for key {} field {}: {}", key, field, e.what());
            throw DBException(e.what());
        }
    }

    std::string RedisConnection::hget(const std::string &key, const std::string &field) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            auto value = redis_->hget(key, field);
            return value ? *value : "";
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Redis HGET failed for key {} field {}: {}", key, field, e.what());
            throw DBException(e.what());
        }
    }

    std::unordered_map<std::string, std::string> RedisConnection::hgetall(const std::string &key) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            std::unordered_map<std::string, std::string> result;
            redis_->hgetall(key, std::inserter(result, result.end()));
            return result;
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Redis HGETALL failed for key {}: {}", key, e.what());
            throw DBException(e.what());
        }
    }

    void RedisConnection::expire(const std::string &key, std::chrono::seconds ttl) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            redis_->expire(key, ttl);
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Redis EXPIRE failed for key {}: {}", key, e.what());
            throw DBException(e.what());
        }
    }

    std::vector<std::string> RedisConnection::scan_keys(const std::string &pattern, const size_t count) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            std::vector<std::string> keys;
            sw::redis::Cursor cursor = 0;
            
            do
            {
                std::vector<std::string> batch_keys;
                cursor = redis_->scan(cursor, pattern, (long long)count, std::back_inserter(batch_keys));
                
                for (const auto &key : batch_keys)
                {
                    keys.push_back(key);
                }
            } while (cursor != 0);
            
            return keys;
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Redis SCAN failed for pattern {}: {}", pattern, e.what());
            throw DBException(e.what());
        }
    }

} // namespace zhttp::zdb
