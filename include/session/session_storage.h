#pragma once
#include "session.h"

namespace zhttp::zsession
{
    class Storage
    {
    public:
        virtual ~Storage() = default;

        // 存储会话
        virtual void store(const std::shared_ptr<Session> &session) = 0;

        // 加载会话
        virtual std::shared_ptr<Session> load(const std::string &session_id) = 0;

        // 删除会话
        virtual void remove(const std::string &session_id) = 0;

        virtual void clear_expired() = 0; // 清除过期会话
    };

} // namespace zhttp::zsession
