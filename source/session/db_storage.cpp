#include "../../include/session/db_storage.h"
#include <nlohmann/json.hpp>
#include <chrono>

namespace zhttp::zsession
{
    // 存储会话到数据库
    void DbSessionStorage::store(const std::shared_ptr<Session> &session)
    {
        const auto conn = pool_.get_connection();
        const nlohmann::json j = session->get_attributes_json();
        std::string attrs = j.dump();
        int64_t expiry = std::chrono::duration_cast<std::chrono::seconds>(
            session->get_expiry_time().time_since_epoch()).count();

        const std::string sql = "REPLACE INTO sessions (session_id, attributes, expiry) VALUES (?, ?, ?)";
        conn->execute_update(sql, session->get_session_id(), attrs, expiry);
    }

    // 从数据库加载会话
    std::shared_ptr<Session> DbSessionStorage::load(const std::string &session_id)
    {
        const auto conn = pool_.get_connection();
        const std::string sql = "SELECT attributes, expiry FROM sessions WHERE session_id = ?";
        if (const auto result = conn->execute_query(sql, session_id); !result.empty())
        {
            const auto &row = result[0]; // 获取第一行数据
            if (row.size() < 2) return nullptr;
            std::string attrs = row[0];
            const int64_t expiry = std::stoll(row[1]);
            auto session = std::make_shared<Session>(session_id);
            nlohmann::json j = nlohmann::json::parse(attrs, nullptr, false);
            if (!j.is_object()) return nullptr;
            for (auto it = j.begin(); it != j.end(); ++it)
            {
                session->set_attribute(it.key(), it.value().get<std::string>());
            }
            session->set_expiry_time(std::chrono::system_clock::time_point(std::chrono::seconds(expiry)));
            return session;
        }
        return nullptr;
    }

    // 删除会话
    void DbSessionStorage::remove(const std::string &session_id)
    {
        const auto conn = pool_.get_connection();
        const std::string sql = "DELETE FROM sessions WHERE session_id = ?";
        conn->execute_update(sql, session_id);
    }

    // 清除所有过期会话
    void DbSessionStorage::clear_expired()
    {
        const auto conn = pool_.get_connection();
        int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        const std::string sql = "DELETE FROM sessions WHERE expiry < ?";
        conn->execute_update(sql, now);
    }
} // namespace zhttp::zsession