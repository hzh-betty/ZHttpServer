#include "../../include/session/session.h"
#include "../../include/log/logger.h"

namespace zhttp::zsession
{
    Session::Session(std::string session_id, uint32_t timeout)
        : session_id_(std::move(session_id)), timeout_(timeout)
    {
        refresh();
        ZHTTP_LOG_DEBUG("Session {} created with timeout {} seconds", session_id_, timeout_);
    }

    // 获取会话ID
    const std::string &Session::get_session_id() const
    {
        return session_id_;
    }


    // 设置与提取会话属性
    void Session::set_attribute(const std::string &key, const std::string &value)
    {
        ZHTTP_LOG_DEBUG("Setting attribute '{}' for session {}", key, session_id_);
        attributes_[key] = value;
    }

    std::string Session::get_attribute(const std::string &key) const
    {
        ZHTTP_LOG_DEBUG("Getting attribute '{}' for session {}", key, session_id_);
        
        if (const auto it = attributes_.find(key); it != attributes_.end())
        {
            return it->second;
        }
        
        ZHTTP_LOG_DEBUG("Attribute '{}' not found in session {}", key, session_id_);
        return "";
    }

    // 刷新过期时间
    void Session::refresh()
    {
        auto old_expiry = expiry_time_;
        expiry_time_ = std::chrono::system_clock::now() + std::chrono::seconds(timeout_);
        
        ZHTTP_LOG_DEBUG("Session {} refreshed, new expiry time updated", session_id_);
    }

    // 检查会话是否过期
    bool Session::is_expired() const
    {
        bool expired = std::chrono::system_clock::now() > expiry_time_;
        if (expired)
        {
            ZHTTP_LOG_DEBUG("Session {} has expired", session_id_);
        }
        return expired;
    }

    // 移除会话属性
    void Session::remove_attribute(const std::string &key)
    {
        ZHTTP_LOG_DEBUG("Removing attribute '{}' from session {}", key, session_id_);
        
        auto removed = attributes_.erase(key);
        if (removed > 0)
        {
            ZHTTP_LOG_DEBUG("Attribute '{}' removed from session {}", key, session_id_);
        }
        else
        {
            ZHTTP_LOG_DEBUG("Attribute '{}' not found in session {} for removal", key, session_id_);
        }
    }

    // 清空会话属性
    void Session::clear_attributes()
    {
        size_t count = attributes_.size();
        ZHTTP_LOG_DEBUG("Clearing {} attributes from session {}", count, session_id_);
        
        attributes_.clear();
        
        ZHTTP_LOG_INFO("All attributes cleared from session {}", session_id_);
    }

    nlohmann::json Session::get_attributes_json() const
    {
        ZHTTP_LOG_DEBUG("Converting attributes to JSON for session {}", session_id_);
        
        nlohmann::json j;
        for (const auto &attr : attributes_)
        {
            j[attr.first] = attr.second;
        }
        
        return j;
    }

    std::chrono::system_clock::time_point Session::get_expiry_time() const
    {
        return expiry_time_;
    }

    void Session::set_expiry_time(std::chrono::system_clock::time_point time_point)
    {
        expiry_time_ = time_point;
        ZHTTP_LOG_DEBUG("Session {} expiry time updated", session_id_);
    }

} // namespace zhttp::zsession
