#pragma once
#include "../../include/db_pool/db_pool.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

// 测试数据库连接池参数
static const std::string TEST_HOST = "1.95.159.45";
static const std::string TEST_USER = "betty";
static const std::string TEST_PASSWORD = "betty";
static const std::string TEST_DATABASE = "test";
static const uint32_t POOL_SIZE = 5;

namespace zhttp::zdb
{
    // Fixture：用于初始化和清理连接池
    class DbConnectionPoolTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // 单例初始化
            DbConnectionPool &pool = DbConnectionPool::get_instance();
            // 第二次 init 应当被忽略，不抛异常
            ASSERT_NO_THROW(pool.init(TEST_HOST, TEST_USER, TEST_PASSWORD, TEST_DATABASE, POOL_SIZE));
            ASSERT_NO_THROW(pool.init(TEST_HOST, TEST_USER, TEST_PASSWORD, TEST_DATABASE, POOL_SIZE));
        }

        void TearDown() override
        {
            // 测试结束后不显式销毁单例，由程序退出时自动清理
        }
    };

    // 测试初始化后，能够轮询取出所有连接
    TEST_F(DbConnectionPoolTest, InitAndAcquireAll)
    {
        DbConnectionPool &pool = DbConnectionPool::get_instance();

        std::vector<std::shared_ptr<DbConnection>> conns;
        for (uint32_t i = 0; i < POOL_SIZE; ++i)
        {
            // 每次 get_connection 应当成功返回
            std::shared_ptr<DbConnection> conn;
            ASSERT_NO_THROW(conn = pool.get_connection());
            ASSERT_NE(conn, nullptr);
            // Ping 测试
            EXPECT_TRUE(conn->ping());
            conns.push_back(conn);
        }

        // 此时再取连接，应因池空被阻塞或抛异常
        std::thread extra([&]()
                          {
                              // 尝试在超时内获取（不带超时逻辑），会阻塞，故在后台线程模拟获取
                              // 仅测试调用不抛异常
                              ASSERT_NO_THROW(pool.get_connection());
                          });
        // 后台线程启动后立即分离，不对其结果做更严格判断
        extra.detach();
    }

    // 多线程并发获取和归还连接测试
    TEST_F(DbConnectionPoolTest, ConcurrentAcquireRelease)
    {
        DbConnectionPool &pool = DbConnectionPool::get_instance();
        const int THREAD_COUNT = 10;
        std::vector<std::thread> threads;
        for (int i = 0; i < THREAD_COUNT; ++i)
        {
            threads.emplace_back([&]()
                                 {
                                     // 每个线程重复多次获取和释放
                                     for (int j = 0; j < 10; ++j)
                                     {
                                         auto conn = pool.get_connection();
                                         ASSERT_NE(conn.get(), nullptr);
                                         EXPECT_TRUE(conn->is_valid());
                                         // 执行简单查询
                                         auto res = conn->execute_query("SELECT 1");
                                         ASSERT_NE(res, nullptr);
                                         EXPECT_TRUE(res->next());
                                         EXPECT_EQ(res->getInt(1), 1);
                                         // ResultSet 由调用者负责释放
                                         delete res;
                                     }
                                 });
        }
        for (auto &t: threads)
        {
            t.join();
        }
    }

    // 压力测试：在限定时间内高频率获取连接并执行查询
    TEST_F(DbConnectionPoolTest, StressTest)
    {
        DbConnectionPool &pool = DbConnectionPool::get_instance();
        const int THREAD_COUNT = 20;
        const auto DURATION = std::chrono::seconds(5); // 压测时长
        std::atomic<int> total_ops{0};

        auto worker = [&]()
        {
            auto end_time = std::chrono::steady_clock::now() + DURATION;
            while (std::chrono::steady_clock::now() < end_time)
            {
                auto conn = pool.get_connection();
                if (conn && conn->is_valid())
                {
                    auto res = conn->execute_query("SELECT 1");
                    if (res)
                    {
                        if (res->next() && res->getInt(1) == 1)
                        {
                            total_ops.fetch_add(1, std::memory_order_relaxed);
                        }
                        delete res;
                    }
                }
            }
        };

        std::vector<std::thread> threads;
        for (int i = 0; i < THREAD_COUNT; ++i)
        {
            threads.emplace_back(worker);
        }
        for (auto &t: threads)
        {
            t.join();
        }

        // 检查在压力测试中至少完成一定数量的操作
        EXPECT_GT(total_ops.load(), THREAD_COUNT * 10);
    }

}// namespace zhttp::zdb

