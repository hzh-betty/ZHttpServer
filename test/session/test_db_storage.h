#pragma once
#include "../../session/db_storage.h"
#include <gtest/gtest.h>

namespace zhttp::zsession
{
    // 数据库连接池初始化
    struct DbPoolInit
    {
        DbPoolInit()
        {
            zhttp::zdb::DbConnectionPool::get_instance().init(
                "1.95.159.45", "betty", "betty", "test", 2);
        }
    };

    inline DbPoolInit _db_pool_init;

    class DbSessionStorageTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            storage = std::make_unique<DbSessionStorage>();
        }

        std::unique_ptr<DbSessionStorage> storage;
    };

    TEST_F(DbSessionStorageTest, StoreAndLoadSession)
    {
        const auto session = std::make_shared<Session>("test_session_id", 60);
        session->set_attribute("user", "alice");
        storage->store(session);

        const auto loaded = storage->load("test_session_id");
        ASSERT_NE(loaded, nullptr);
        EXPECT_EQ(loaded->get_session_id(), "test_session_id");
        EXPECT_EQ(loaded->get_attribute("user"), "alice");
    }

    TEST_F(DbSessionStorageTest, RemoveSession)
    {
        const auto session = std::make_shared<Session>("to_remove", 60);
        session->set_attribute("foo", "bar");
        storage->store(session);

        storage->remove("to_remove");
        const auto loaded = storage->load("to_remove");
        EXPECT_EQ(loaded, nullptr);
    }

    TEST_F(DbSessionStorageTest, ClearExpired)
    {
        const auto session = std::make_shared<Session>("expired_id", 1);
        session->set_attribute("k", "v");
        storage->store(session);

        // 等待会话过期
        std::this_thread::sleep_for(std::chrono::seconds(2));
        storage->clear_expired();

        const auto loaded = storage->load("expired_id");
        EXPECT_EQ(loaded, nullptr);
    }
} // namespace zhttp::zsession
