#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>
#include <nlohmann/json.hpp>

namespace zhttp::zsession
{
    class Session
    {
    public:
        explicit Session(std::string session_id, uint32_t timeout = 3600);

        // 获取会话ID
        const std::string &get_session_id() const;

        // 设置与提取会话属性
        void set_attribute(const std::string &key, const std::string &value);

        std::string get_attribute(const std::string &key) const;

        // 刷新过期时间
        void refresh();

        // 检查会话是否过期
        bool is_expired() const;

        // 移除会话属性
        void remove_attribute(const std::string &key);

        // 清空会话属性
        void clear_attributes();

        // 获取json格式会话属性
        nlohmann::json get_attributes_json() const;

        // 获取过期时间
        std::chrono::system_clock::time_point get_expiry_time() const;

        void set_expiry_time(std::chrono::system_clock::time_point time_point);

    private:
        std::string session_id_; // 会话ID
        std::unordered_map<std::string, std::string> attributes_; // 会话属性
        std::chrono::system_clock::time_point expiry_time_; // 过期时间
        uint32_t timeout_; // 会话超时时间（秒）
    };
} // namespace zhttp::zsession

