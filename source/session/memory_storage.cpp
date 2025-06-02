#include "../../include/session/memory_storage.h"
namespace zhttp::zsession
{
    // 存储会话
    void InMemoryStorage::store(const std::shared_ptr<Session> &session)
    {
        sessions_[session->get_session_id()] = session;
    }

    // 加载会话
    std::shared_ptr<Session> InMemoryStorage::load(const std::string &session_id)
    {
        auto it = sessions_.find(session_id);
        if (it != sessions_.end())
        {
            return it->second;
        }
        return nullptr;
    }

    // 删除会话
    void InMemoryStorage::remove(const std::string &session_id)
    {
        sessions_.erase(session_id);
    }

    // 清除所有过期会话
    void InMemoryStorage::clear_expired()
    {
        auto it = sessions_.begin();
        while (it != sessions_.end())
        {
            if (it->second->is_expired())
            {
                it = sessions_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

} // namespace zhttp::zsession


