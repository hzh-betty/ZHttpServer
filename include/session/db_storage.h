#pragma once
#include "../db_pool/db_pool.h"
#include "session_storage.h"

namespace zhttp::zsession
{
    class DbSessionStorage final : public Storage
    {
    public:
        void store(const std::shared_ptr<Session> &session) override;

        std::shared_ptr<Session> load(const std::string &session_id) override;

        void remove(const std::string &session_id) override;

        void clear_expired() override;

    private:
        zdb::DbConnectionPool &pool_ = zdb::DbConnectionPool::get_instance();
    };
} //  namespace zhttp::zsession
