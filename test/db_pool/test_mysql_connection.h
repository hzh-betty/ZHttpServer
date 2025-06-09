#pragma once

#include "../../include/db_pool/mysql_connection.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>

namespace zhttp::zdb
{
    // 全局数据库配置信息
    inline const std::string HOST = "127.0.0.1";
    inline const std::string USER = "betty";
    inline const std::string PASSWORD = "betty";
    inline const std::string DATABASE = "test";

    // 测试 DbConnection 类
    class MysqlConnectionTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            conn_ = std::make_shared<MysqlConnection>(HOST, USER, PASSWORD, DATABASE);
            ASSERT_TRUE(conn_->is_valid()) << "初始化连接失败";
        }

        void TearDown() override
        {
            conn_.reset();
        }

        std::shared_ptr<MysqlConnection> conn_;
    };

    TEST_F(MysqlConnectionTest, TestPing)
    {
        ASSERT_TRUE(conn_->ping());
    }

    TEST_F(MysqlConnectionTest, TestExecuteUpdate)
    {
        conn_->execute_update(
                "CREATE TABLE IF NOT EXISTS gtest_users (id INT PRIMARY KEY AUTO_INCREMENT, name VARCHAR(255))");
        int affected = conn_->execute_update(
                "INSERT INTO gtest_users(name) VALUES(?)", std::string("Alice"));
        ASSERT_EQ(affected, 1);
    }

    TEST_F(MysqlConnectionTest, TestExecuteQuery)
    {
        auto result = conn_->execute_query(
                "SELECT name FROM gtest_users WHERE name = ?", std::string("Alice"));
        ASSERT_FALSE(result.empty());
        ASSERT_EQ(result[0][0], "Alice");
    }

    TEST_F(MysqlConnectionTest, TestReconnect)
    {
        conn_->reconnect();
        ASSERT_TRUE(conn_->is_valid());
    }

    // 并发执行简单查询
    TEST_F(MysqlConnectionTest, ConcurrentExecuteQuery)
    {
        constexpr int kThreads = 8;
        std::atomic<int> success_count{0};
        std::vector<std::thread> threads;

        for (int i = 0; i < kThreads; ++i)
        {
            threads.emplace_back([&]()
                                 {
                                     try
                                     {
                                         auto rows = conn_->execute_query("SELECT 1");
                                         if (!rows.empty() && rows[0][0] == "1")
                                         {
                                             ++success_count;
                                         }
                                     } catch (const std::exception &e)
                                     {
                                         ADD_FAILURE() << "线程查询失败: " << e.what();
                                     }
                                 });
        }

        for (auto &t: threads)
        {
            t.join();
        }

        EXPECT_EQ(success_count, kThreads);
    }

    // 并发 ping 与查询，测试锁竞争
    TEST_F(MysqlConnectionTest, ConcurrentPingAndQuery)
    {
        constexpr int kThreads = 4;
        std::atomic<int> ping_success{0};
        std::atomic<int> query_success{0};
        std::vector<std::thread> threads;

        for (int i = 0; i < kThreads; ++i)
        {
            threads.emplace_back([&]()
                                 {
                                     for (int j = 0; j < 5; ++j)
                                     {
                                         try
                                         {
                                             if (conn_->ping()) ++ping_success;
                                         } catch (...)
                                         {
                                             ADD_FAILURE() << "线程 ping 异常";
                                         }
                                         try
                                         {
                                             auto rows = conn_->execute_query("SELECT 1");
                                             if (!rows.empty() && rows[0][0] == "1") ++query_success;
                                         } catch (const std::exception &e)
                                         {
                                             ADD_FAILURE() << "线程查询失败: " << e.what();
                                         }
                                     }
                                 });
        }

        for (auto &t: threads)
        {
            t.join();
        }

        EXPECT_EQ(ping_success, kThreads * 5);
        EXPECT_EQ(query_success, kThreads * 5);
    }
}