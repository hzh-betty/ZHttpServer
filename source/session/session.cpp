#include "../../include/session/session.h"

namespace zhttp::zsession
{
    Session::Session(std::string session_id, uint32_t timeout)
        : session_id_(std::move(session_id)), timeout_(timeout)
    {
        refresh();
    }

    // 获取会话ID
    const std::string &Session::get_session_id() const
    {
        return session_id_;
    }


    // 设置与提取会话属性
    void Session::set_attribute(const std::string &key, const std::string &value)
    {
        attributes_[key] = value;
    }

    std::string Session::get_attribute(const std::string &key) const
    {
        return attributes_.count(key) ? attributes_.at(key) : "";
    }

    // 刷新过期时间
    void Session::refresh()
    {
        expiry_time_ = std::chrono::system_clock::now() + std::chrono::seconds(timeout_);
    }

    // 检查会话是否过期
    bool Session::is_expired() const
    {
        return std::chrono::system_clock::now() > expiry_time_;
    }

    // 移除会话属性
    void Session::remove_attribute(const std::string &key)
    {
        attributes_.erase(key);
    }

    // 清空会话属性
    void Session::clear_attributes()
    {
        attributes_.clear();
    }

    nlohmann::json Session::get_attributes_json() const
    {
        nlohmann::json j;
        for (const auto &[k, v]: attributes_)
        {
            j[k] = v;
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
    }
} // namespace zhttp::zsession
