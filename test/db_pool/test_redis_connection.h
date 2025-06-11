#pragma once
#include <gtest/gtest.h>
#include "../../include/db_pool/redis_connection.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

namespace zhttp::zdb
{
    class RedisConnectionTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            conn = new RedisConnection("127.0.0.1", 6379, "", 0, 2000);
        }

        void TearDown() override
        {
            delete conn;
        }

        RedisConnection *conn = nullptr;
    };

    TEST_F(RedisConnectionTest, IsValid)
    {
        EXPECT_TRUE(conn->is_valid());
    }

    TEST_F(RedisConnectionTest, Ping)
    {
        EXPECT_TRUE(conn->ping());
    }

    TEST_F(RedisConnectionTest, SetGetDel)
    {
        std::string key = "gtest_key";
        std::string value = "gtest_value";
        conn->set(key, value);
        EXPECT_EQ(conn->get(key), value);
        EXPECT_TRUE(conn->exists(key));
        EXPECT_TRUE(conn->del(key));
        EXPECT_FALSE(conn->exists(key));
    }

    TEST_F(RedisConnectionTest, Expire)
    {
        std::string key = "expire_key";
        conn->set(key, "1");
        conn->expire(key, std::chrono::seconds(1));
        std::this_thread::sleep_for(std::chrono::seconds(2));
        EXPECT_FALSE(conn->exists(key));
    }

    TEST_F(RedisConnectionTest, HashSetGet)
    {
        std::string key = "gtest_hash";
        std::string field = "field1";
        std::string value = "val1";
        conn->hset(key, field, value);
        EXPECT_EQ(conn->hget(key, field), value);
        auto all = conn->hgetall(key);
        EXPECT_EQ(all[field], value);
        EXPECT_TRUE(conn->del(key));
    }

    TEST_F(RedisConnectionTest, ScanKeys)
    {
        std::string key1 = "scan_key1";
        std::string key2 = "scan_key2";
        conn->set(key1, "1");
        conn->set(key2, "2");
        auto keys = conn->scan_keys("scan_key*");
        EXPECT_NE(std::find(keys.begin(), keys.end(), key1), keys.end());
        EXPECT_NE(std::find(keys.begin(), keys.end(), key2), keys.end());
        EXPECT_TRUE(conn->del(key1));
        EXPECT_TRUE(conn->del(key2));
    }

    TEST_F(RedisConnectionTest, Reconnect)
    {
        EXPECT_NO_THROW(conn->reconnect());
        EXPECT_TRUE(conn->is_valid());
    }

    TEST_F(RedisConnectionTest, ConcurrentSetGet)
    {
        constexpr int thread_count = 10;
        constexpr int ops_per_thread = 50;
        std::atomic<int> success_count{0};
        std::vector<std::thread> threads;

        for (int t = 0; t < thread_count; ++t)
        {
            threads.emplace_back([&, t]() {
                for (int i = 0; i < ops_per_thread; ++i)
                {
                    std::string key = "concurrent_key_" + std::to_string(t) + "_" + std::to_string(i);
                    std::string value = "val_" + std::to_string(i);
                    try
                    {
                        conn->set(key, value);
                        if (conn->get(key) == value)
                            ++success_count;
                        EXPECT_TRUE(conn->del(key));
                    }
                    catch (...)
                    {
                        // 忽略异常，计数不增加
                    }
                }
            });
        }
        for (auto &th : threads) th.join();
        EXPECT_EQ(success_count, thread_count * ops_per_thread);
    }

} // namespace zhttp::zdb