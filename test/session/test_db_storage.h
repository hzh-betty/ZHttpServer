#pragma once
#include <gtest/gtest.h>
#include "db_pool/redis_pool.h"
#include "session/db_storage.h"
#include "session/session.h"
#include <thread>
#include <chrono>

namespace zhttp::zsession
{
    class DbSessionStorageTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            zdb::RedisConnectionPool::get_instance().init("127.0.0.1", 6379, "", 0, 3, 2000);
            storage = std::make_unique<DbSessionStorage>();
            session_id = "gtest_session_id";
            session = std::make_shared<Session>(session_id);
            session->set_attribute("user", "alice");
            session->set_attribute("role", "admin");
            session->set_expiry_time(std::chrono::system_clock::now() + std::chrono::seconds(60));
        }

        void TearDown() override
        {
            storage->remove(session_id);
        }

        std::unique_ptr<DbSessionStorage> storage;
        std::shared_ptr<Session> session;
        std::string session_id;
    };

    TEST_F(DbSessionStorageTest, StoreAndLoad)
    {
        storage->store(session);
        auto loaded = storage->load(session_id);
        ASSERT_NE(loaded, nullptr);
        EXPECT_EQ(loaded->get_session_id(), session_id);
        EXPECT_EQ(loaded->get_attribute("user"), "alice");
        EXPECT_EQ(loaded->get_attribute("role"), "admin");
    }

    TEST_F(DbSessionStorageTest, Remove)
    {
        storage->store(session);
        storage->remove(session_id);
        auto loaded = storage->load(session_id);
        EXPECT_EQ(loaded, nullptr);
    }

    TEST_F(DbSessionStorageTest, ExpiredSessionNotLoaded)
    {
        session->set_expiry_time(std::chrono::system_clock::now() + std::chrono::seconds(1));
        storage->store(session);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        auto loaded = storage->load(session_id);
        EXPECT_EQ(loaded, nullptr);
    }

    TEST_F(DbSessionStorageTest, ClearExpired)
    {
        // 存储一个即将过期的session
        session->set_expiry_time(std::chrono::system_clock::now() + std::chrono::seconds(1));
        storage->store(session);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        storage->clear_expired();
        auto loaded = storage->load(session_id);
        EXPECT_EQ(loaded, nullptr);
    }
} // namespace zhttp::zsession
