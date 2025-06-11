#include "../../include/session/memory_storage.h"
#include "../../include/log/logger.h"

namespace zhttp::zsession
{
    // 存储会话
    void InMemoryStorage::store(const std::shared_ptr<Session> &session)
    {
        ZHTTP_LOG_DEBUG("Storing session: {}", session->get_session_id());
        sessions_[session->get_session_id()] = session;
        ZHTTP_LOG_INFO("Session {} stored successfully, total sessions: {}", 
                      session->get_session_id(), sessions_.size());
    }

    // 加载会话
    std::shared_ptr<Session> InMemoryStorage::load(const std::string &session_id)
    {
        ZHTTP_LOG_DEBUG("Loading session: {}", session_id);
        
        if (const auto it = sessions_.find(session_id); it != sessions_.end())
        {
            if (it->second->is_expired())
            {
                ZHTTP_LOG_WARN("Session {} has expired, removing from storage", session_id);
                sessions_.erase(it);
                return nullptr;
            }
            
            ZHTTP_LOG_DEBUG("Session {} loaded successfully", session_id);
            return it->second;
        }
        
        ZHTTP_LOG_DEBUG("Session {} not found in storage", session_id);
        return nullptr;
    }

    // 删除会话
    void InMemoryStorage::remove(const std::string &session_id)
    {
        ZHTTP_LOG_DEBUG("Removing session: {}", session_id);
        
        auto removed_count = sessions_.erase(session_id);
        if (removed_count > 0)
        {
            ZHTTP_LOG_INFO("Session {} removed successfully, remaining sessions: {}", 
                          session_id, sessions_.size());
        }
        else
        {
            ZHTTP_LOG_DEBUG("Session {} not found for removal", session_id);
        }
    }

    // 清除所有过期会话
    void InMemoryStorage::clear_expired()
    {
        ZHTTP_LOG_DEBUG("Starting expired session cleanup, current sessions: {}", sessions_.size());
        
        size_t initial_count = sessions_.size();
        auto it = sessions_.begin();
        size_t removed_count = 0;
        
        while (it != sessions_.end())
        {
            if (it->second->is_expired())
            {
                ZHTTP_LOG_DEBUG("Removing expired session: {}", it->first);
                it = sessions_.erase(it);
                removed_count++;
            }
            else
            {
                ++it;
            }
        }
        
        ZHTTP_LOG_INFO("Expired session cleanup completed: removed {}, remaining {}", 
                      removed_count, sessions_.size());
    }

} // namespace zhttp::zsession


