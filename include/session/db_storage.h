#pragma once
#include "session_storage.h"

namespace zhttp::zsession
{
    class DbSessionStorage final : public Storage
    {
    public:
        DbSessionStorage() = default;
        ~DbSessionStorage() override = default;

        void store(const std::shared_ptr<Session> &session) override;

        std::shared_ptr<Session> load(const std::string &session_id) override;

        void remove(const std::string &session_id) override;

        void clear_expired() override;
    };
} //  namespace zhttp::zsession
