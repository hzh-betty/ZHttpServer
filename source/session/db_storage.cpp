#include "session/db_storage.h"
#include "db_pool/redis_pool.h"
#include "log/http_logger.h"
#include <nlohmann/json.hpp>
#include <chrono>

namespace zhttp::zsession
{
    // 存储会话到Redis
    void DbSessionStorage::store(const std::shared_ptr<Session> &session)
    {
        ZHTTP_LOG_DEBUG("Storing session {} to Redis", session->get_session_id());
        
        try
        {
            auto &redis_pool = zdb::RedisConnectionPool::get_instance();
            const auto conn = redis_pool.get_connection();
            const nlohmann::json j = session->get_attributes_json();
            const std::string attrs = j.dump();

            const auto expiry_time = session->get_expiry_time();
            const auto now = std::chrono::system_clock::now();
            const auto ttl = std::chrono::duration_cast<std::chrono::seconds>(expiry_time - now);
            
            if (ttl.count() <= 0)
            {
                ZHTTP_LOG_WARN("Session {} has already expired, not storing to Redis", session->get_session_id());
                return;
            }

            const std::string key = "session:" + session->get_session_id();
            
            // 使用Redis Hash存储会话数据
            conn->hset(key, "attributes", attrs);
            conn->hset(key, "expiry", std::to_string(
                std::chrono::duration_cast<std::chrono::seconds>(expiry_time.time_since_epoch()).count()));
            
            // 设置过期时间
            conn->expire(key, ttl);
            
            ZHTTP_LOG_INFO("Session {} stored to Redis successfully with TTL {} seconds", 
                          session->get_session_id(), ttl.count());
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Failed to store session {} to Redis: {}", 
                           session->get_session_id(), e.what());
            throw;
        }
    }

    // 从Redis加载会话
    std::shared_ptr<Session> DbSessionStorage::load(const std::string &session_id)
    {
        ZHTTP_LOG_DEBUG("Loading session {} from Redis", session_id);
        
        try
        {
            auto &redis_pool = zdb::RedisConnectionPool::get_instance();
            const auto conn = redis_pool.get_connection();
            const std::string key = "session:" + session_id;
            
            // 检查key是否存在
            if (!conn->exists(key))
            {
                ZHTTP_LOG_DEBUG("Session {} not found in Redis", session_id);
                return nullptr;
            }

            // 获取会话数据
            auto result = conn->hgetall(key);
            if (result.empty())
            {
                ZHTTP_LOG_DEBUG("Session {} data is empty in Redis", session_id);
                return nullptr;
            }

            const auto attrs_it = result.find("attributes");
            const auto expiry_it = result.find("expiry");
            
            if (attrs_it == result.end() || expiry_it == result.end())
            {
                ZHTTP_LOG_WARN("Incomplete session data for {} in Redis", session_id);
                return nullptr;
            }

            // 检查是否过期
            const int64_t expiry = std::stoll(expiry_it->second);
            const auto expiry_time = std::chrono::system_clock::time_point(std::chrono::seconds(expiry));
            if (expiry_time < std::chrono::system_clock::now())
            {
                ZHTTP_LOG_WARN("Session {} has expired, removing from Redis", session_id);
                remove(session_id);
                return nullptr;
            }

            // 创建会话对象
            auto session = std::make_shared<Session>(session_id);
            
            // 解析属性
            nlohmann::json j = nlohmann::json::parse(attrs_it->second, nullptr, false);
            if (!j.is_object())
            {
                ZHTTP_LOG_ERROR("Invalid JSON format for session {} attributes in Redis", session_id);
                return nullptr;
            }
            
            for (auto it = j.begin(); it != j.end(); ++it)
            {
                session->set_attribute(it.key(), it.value().get<std::string>());
            }
            session->set_expiry_time(expiry_time);
            
            ZHTTP_LOG_DEBUG("Session {} loaded from Redis successfully", session_id);
            return session;
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Failed to load session {} from Redis: {}", session_id, e.what());
            return nullptr;
        }
    }

    // 删除会话
    void DbSessionStorage::remove(const std::string &session_id)
    {
        ZHTTP_LOG_DEBUG("Removing session {} from Redis", session_id);
        
        try
        {
            auto &redis_pool = zdb::RedisConnectionPool::get_instance();
            const auto conn = redis_pool.get_connection();
            const std::string key = "session:" + session_id;

            if (conn->del(key))
            {
                ZHTTP_LOG_INFO("Session {} removed from Redis successfully", session_id);
            }
            else
            {
                ZHTTP_LOG_DEBUG("Session {} not found for removal in Redis", session_id);
            }
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Failed to remove session {} from Redis: {}", session_id, e.what());
            throw;
        }
    }

    // 清除所有过期会话
    void DbSessionStorage::clear_expired()
    {
        ZHTTP_LOG_DEBUG("Starting Redis expired session cleanup");
        
        try
        {
            auto &redis_pool = zdb::RedisConnectionPool::get_instance();
            const auto conn = redis_pool.get_connection();
            const std::string pattern = "session:*";
            
            // 使用SCAN命令遍历所有session key
            auto keys = conn->scan_keys(pattern, 100);
            size_t removed_count = 0;
            
            for (const auto &key : keys)
            {
                try
                {
                    // 获取会话数据检查是否过期
                    auto result = conn->hgetall(key);

                    if (auto expiry_it = result.find("expiry"); expiry_it != result.end())
                    {
                        int64_t expiry = std::stoll(expiry_it->second);

                        if (auto expiry_time = std::chrono::system_clock::time_point(std::chrono::seconds(expiry));
                            expiry_time < std::chrono::system_clock::now())
                        {
                            if (!conn->del(key))
                            {
                                ZHTTP_LOG_WARN("Removed session {} from Redis failed", key);
                                continue;
                            }
                            removed_count++;
                            ZHTTP_LOG_DEBUG("Removed expired session key: {}", key);
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    ZHTTP_LOG_WARN("Error checking expiry for key {}: {}", key, e.what());
                }
            }
            
            ZHTTP_LOG_INFO("Redis expired session cleanup completed, removed {} sessions", removed_count);
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Failed to cleanup expired sessions from Redis: {}", e.what());
            throw;
        }
    }
    
} // namespace zhttp::zsession