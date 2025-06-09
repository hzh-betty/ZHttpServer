#pragma once
#include <gtest/gtest.h>
#include "../../include/db_pool/redis_pool.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

namespace zhttp::zdb
{
    class RedisPoolTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // 避免重复初始化
            if (!RedisConnectionPool::get_instance().is_initialized())
            {
                RedisConnectionPool::get_instance().init("127.0.0.1", 6379, "", 0, 3, 2000);
            }
        }
        void TearDown() override {}
    };

    TEST_F(RedisPoolTest, PoolInitialization)
    {
        EXPECT_TRUE(RedisConnectionPool::get_instance().is_initialized());
        EXPECT_EQ(RedisConnectionPool::get_instance().get_pool_size(), 3);
    }

    TEST_F(RedisPoolTest, GetConnection)
    {
        auto conn = RedisConnectionPool::get_instance().get_connection();
        ASSERT_TRUE(conn != nullptr);
        EXPECT_TRUE(conn->is_valid());
        conn->set("pool_test_key", "pool_test_value");
        EXPECT_EQ(conn->get("pool_test_key"), "pool_test_value");
        EXPECT_TRUE(conn->del("pool_test_key"));
    }

    TEST_F(RedisPoolTest, PoolSizeAfterGetAndReturn)
    {
        size_t before = RedisConnectionPool::get_instance().get_pool_size();
        {
            auto conn = RedisConnectionPool::get_instance().get_connection();
            EXPECT_TRUE(conn->is_valid());
            // 连接归还后，池大小应恢复
        }
        size_t after = RedisConnectionPool::get_instance().get_pool_size();
        EXPECT_EQ(before, after);
    }

    TEST_F(RedisPoolTest, RepeatedInit)
    {
        // 再次初始化不会抛异常，也不会改变池状态
        RedisConnectionPool::get_instance().init("127.0.0.1", 6379, "", 0, 3, 2000);
        EXPECT_TRUE(RedisConnectionPool::get_instance().is_initialized());
    }

    TEST_F(RedisPoolTest, ConcurrentGetConnection)
    {
        constexpr int thread_count = 10;
        constexpr int ops_per_thread = 20;
        std::atomic<int> success_count{0};
        std::vector<std::thread> threads;

        for (int t = 0; t < thread_count; ++t)
        {
            threads.emplace_back([&, t]() {
                for (int i = 0; i < ops_per_thread; ++i)
                {
                    try
                    {
                        auto conn = RedisConnectionPool::get_instance().get_connection();
                        std::string key = "pool_concurrent_" + std::to_string(t) + "_" + std::to_string(i);
                        std::string value = "v" + std::to_string(i);
                        conn->set(key, value);
                        if (conn->get(key) == value)
                            ++success_count;
                        EXPECT_TRUE(conn->del(key));
                    }
                    catch (...)
                    {
                        // 忽略异常
                    }
                }
            });
        }
        for (auto &th : threads) th.join();
        EXPECT_EQ(success_count, thread_count * ops_per_thread);
    }
}