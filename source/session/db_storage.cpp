#include "../../include/session/db_storage.h"
#include "../../include/log/logger.h"
#include <nlohmann/json.hpp>
#include <chrono>

namespace zhttp::zsession
{
    // 存储会话到数据库
    void DbSessionStorage::store(const std::shared_ptr<Session> &session)
    {
        ZHTTP_LOG_DEBUG("Storing session {} to database", session->get_session_id());
        
        try
        {
            const auto conn = pool_.get_connection();
            const nlohmann::json j = session->get_attributes_json();
            std::string attrs = j.dump();
            int64_t expiry = std::chrono::duration_cast<std::chrono::seconds>(
                session->get_expiry_time().time_since_epoch()).count();

            const std::string sql = "REPLACE INTO sessions (session_id, attributes, expiry) VALUES (?, ?, ?)";
            conn->execute_update(sql, session->get_session_id(), attrs, expiry);
            
            ZHTTP_LOG_INFO("Session {} stored to database successfully", session->get_session_id());
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Failed to store session {} to database: {}", 
                           session->get_session_id(), e.what());
            throw;
        }
    }

    // 从数据库加载会话
    std::shared_ptr<Session> DbSessionStorage::load(const std::string &session_id)
    {
        ZHTTP_LOG_DEBUG("Loading session {} from database", session_id);
        
        try
        {
            const auto conn = pool_.get_connection();
            const std::string sql = "SELECT attributes, expiry FROM sessions WHERE session_id = ?";
            
            if (const auto result = conn->execute_query(sql, session_id); !result.empty())
            {
                const auto &row = result[0];
                if (row.size() < 2) 
                {
                    ZHTTP_LOG_WARN("Incomplete session data for {}", session_id);
                    return nullptr;
                }
                
                std::string attrs = row[0];
                const int64_t expiry = std::stoll(row[1]);
                auto session = std::make_shared<Session>(session_id);
                
                // 检查是否过期
                auto expiry_time = std::chrono::system_clock::time_point(std::chrono::seconds(expiry));
                if (expiry_time < std::chrono::system_clock::now())
                {
                    ZHTTP_LOG_WARN("Session {} has expired, removing from database", session_id);
                    remove(session_id);
                    return nullptr;
                }
                
                nlohmann::json j = nlohmann::json::parse(attrs, nullptr, false);
                if (!j.is_object())
                {
                    ZHTTP_LOG_ERROR("Invalid JSON format for session {} attributes", session_id);
                    return nullptr;
                }
                
                for (auto it = j.begin(); it != j.end(); ++it)
                {
                    session->set_attribute(it.key(), it.value().get<std::string>());
                }
                session->set_expiry_time(expiry_time);
                
                ZHTTP_LOG_DEBUG("Session {} loaded from database successfully", session_id);
                return session;
            }
            
            ZHTTP_LOG_DEBUG("Session {} not found in database", session_id);
            return nullptr;
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Failed to load session {} from database: {}", session_id, e.what());
            return nullptr;
        }
    }

    // 删除会话
    void DbSessionStorage::remove(const std::string &session_id)
    {
        ZHTTP_LOG_DEBUG("Removing session {} from database", session_id);
        
        try
        {
            const auto conn = pool_.get_connection();
            const std::string sql = "DELETE FROM sessions WHERE session_id = ?";
            conn->execute_update(sql, session_id);
            
            ZHTTP_LOG_INFO("Session {} removed from database successfully", session_id);
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Failed to remove session {} from database: {}", session_id, e.what());
            throw;
        }
    }

    // 清除所有过期会话
    void DbSessionStorage::clear_expired()
    {
        ZHTTP_LOG_DEBUG("Starting database expired session cleanup");
        
        try
        {
            const auto conn = pool_.get_connection();
            int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            const std::string sql = "DELETE FROM sessions WHERE expiry < ?";
            
            auto affected_rows = conn->execute_update(sql, now);
            ZHTTP_LOG_INFO("Database expired session cleanup completed, removed {} sessions", affected_rows);
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Failed to cleanup expired sessions from database: {}", e.what());
            throw;
        }
    }
    
} // namespace zhttp::zsession