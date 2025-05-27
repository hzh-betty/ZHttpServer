#pragma once

#include "../../include/db_pool/db_pool.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

namespace zhttp::zdb
{
    class DbConnectionPoolTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            DbConnectionPool::get_instance().init(HOST, USER, PASSWORD, DATABASE, 8);
        }
    };

    // 测试获取连接是否成功
    TEST_F(DbConnectionPoolTest, CanGetConnection)
    {
        auto conn = DbConnectionPool::get_instance().get_connection();
        ASSERT_NE(conn, nullptr);
        ASSERT_TRUE(conn->is_valid());
    }

    // 测试 execute_query 能否正常工作
    TEST_F(DbConnectionPoolTest, CanExecuteSimpleQuery)
    {
        auto conn = DbConnectionPool::get_instance().get_connection();
        auto result = conn->execute_query("SELECT 1");
        ASSERT_EQ(result.size(), 1);
        ASSERT_EQ(result[0].size(), 1);
        ASSERT_EQ(result[0][0], "1");
    }

    // 测试连接是否自动回收
    TEST_F(DbConnectionPoolTest, ConnectionAutoRecycle)
    {
        {
            auto conn = DbConnectionPool::get_instance().get_connection();
            ASSERT_TRUE(conn->is_valid());
        } // 作用域结束，连接应被回收

        // 再次获取
        auto conn2 = DbConnectionPool::get_instance().get_connection();
        ASSERT_TRUE(conn2->is_valid());
    }

    // 测试连接失效后能否自动重连
    TEST_F(DbConnectionPoolTest, ReconnectAfterFailure)
    {
        auto conn = DbConnectionPool::get_instance().get_connection();
        // 模拟失效：直接重连
        conn->reconnect();
        ASSERT_TRUE(conn->is_valid());
    }


    // 压力测试：并发获取连接并执行 SQL
    TEST_F(DbConnectionPoolTest, StressTestWithMultipleThreads)
    {
        constexpr int THREAD_COUNT = 10;
        constexpr int TASKS_PER_THREAD = 500;
        std::vector<std::thread> threads;
        std::atomic<int> success_count = 0;

        for (int i = 0; i < THREAD_COUNT; ++i)
        {

            threads.emplace_back([&]()
                                 {
                                     for (int j = 0; j < TASKS_PER_THREAD; ++j)
                                     {

                                         try
                                         {
                                             auto conn = DbConnectionPool::get_instance().get_connection();
                                             auto res = conn->execute_query("SELECT 1");
                                             if (!res.empty() && res[0][0] == "1")
                                             {
                                                 ++success_count;
                                             }
                                         }
                                         catch (const std::exception &e)
                                         {
                                             FAIL() << "Exception in stress test: " << e.what();
                                         }
                                     }
                                 });
        }

        for (auto &t: threads)
        {
            t.join();
        }

        ASSERT_EQ(success_count, THREAD_COUNT * TASKS_PER_THREAD);
    }

}// namespace zhttp::zdb

