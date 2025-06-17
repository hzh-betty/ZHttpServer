#pragma once
#include "session_storage.h"

namespace zhttp::zsession
{
    class InMemoryStorage final : public SessionStorage
    {
    public:
        InMemoryStorage() = default;

        ~InMemoryStorage() override = default;

        // 存储会话
        void store(const std::shared_ptr<Session> &session) override;

        // 加载会话
        std::shared_ptr<Session> load(const std::string &session_id) override;

        // 删除会话
        void remove(const std::string &session_id) override;

        // 清除过期会话
        void clear_expired() override;

    private:
        std::unordered_map<std::string, std::shared_ptr<Session> > sessions_; // 存储会话的哈希表
    };
} //  namespace zhttp::zsession
