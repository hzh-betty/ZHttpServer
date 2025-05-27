#pragma once
#include "../../include/db_pool/db_connection.h"
#include  <gtest/gtest.h>
#include  <thread>


namespace zhttp::zdb
{
    class DbConnectionTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // 测试用的数据库连接信息
            host = "1.95.159.45";
            user = "betty";
            password = "betty";
            database = "test";

            // 创建测试表（如果不存在）
            try
            {
                DbConnection setupConn(host, user, password, database);
                setupConn.execute_update("DROP TABLE IF EXISTS test_table");
                setupConn.execute_update("CREATE TABLE test_table (id INT PRIMARY KEY, name VARCHAR(50))");
                setupConn.execute_update("INSERT INTO test_table VALUES (1, 'Test1'), (2, 'Test2')");
            } catch (const DBException &e)
            {
                FAIL() << "Failed to setup test table: " << e.what();
            }
        }

        void TearDown() override
        {
            // 清理测试表
            try
            {
                DbConnection cleanupConn(host, user, password, database);
                cleanupConn.execute_update("DROP TABLE IF EXISTS test_table");
            } catch (...)
            {
                // 忽略清理错误
            }
        }

        std::string host;
        std::string user;
        std::string password;
        std::string database;
    };

    // 测试构造函数和析构函数
    TEST_F(DbConnectionTest, ConstructionAndDestruction)
    {
        EXPECT_NO_THROW({
                            DbConnection conn(host, user, password, database);
                            EXPECT_TRUE(conn.is_valid());
                        });
    }

    // 测试无效连接
    TEST_F(DbConnectionTest, InvalidConnection)
    {
        EXPECT_THROW({
                         DbConnection conn("invalid_host", user, password, database);
                     }, DBException);
    }

    // 测试连接有效性检查
    TEST_F(DbConnectionTest, IsValid)
    {
        DbConnection conn(host, user, password, database);
        EXPECT_TRUE(conn.is_valid());

        // 模拟连接失效（实际可能无法模拟，这里只是测试行为）
        // 通常需要mock来测试这种情况
    }

    // 测试ping方法
    TEST_F(DbConnectionTest, Ping)
    {
        DbConnection conn(host, user, password, database);
        EXPECT_TRUE(conn.ping());
    }

    // 测试重连功能
    TEST_F(DbConnectionTest, Reconnect)
    {
        DbConnection conn(host, user, password, database);
        EXPECT_NO_THROW(conn.reconnect());
        EXPECT_TRUE(conn.is_valid());
    }

    // 测试执行查询
    TEST_F(DbConnectionTest, ExecuteQuery)
    {
        DbConnection conn(host, user, password, database);
        auto result = conn.execute_query("SELECT * FROM test_table WHERE id = ?", 1);
        ASSERT_NE(result, nullptr);
        EXPECT_TRUE(result->next());
        EXPECT_EQ(result->getString("name"), "Test1");
        delete result;
    }

    // 测试执行更新
    TEST_F(DbConnectionTest, ExecuteUpdate)
    {
        DbConnection conn(host, user, password, database);
        int affected = conn.execute_update("INSERT INTO test_table VALUES (3, 'Test3')");
        EXPECT_EQ(affected, 1);

        // 验证插入是否成功
        auto result = conn.execute_query("SELECT * FROM test_table WHERE id = 3");
        ASSERT_NE(result, nullptr);
        EXPECT_TRUE(result->next());
        delete result;
    }

    // 测试参数绑定
    TEST_F(DbConnectionTest, ParameterBinding)
    {
        DbConnection conn(host, user, password, database);

        // 测试int参数
        auto result1 = conn.execute_query("SELECT * FROM test_table WHERE id = ?", 1);
        ASSERT_NE(result1, nullptr);
        EXPECT_TRUE(result1->next());
        delete result1;

        // 测试string参数
        auto result2 = conn.execute_query("SELECT * FROM test_table WHERE name = ?", std::string("Test1"));
        ASSERT_NE(result2, nullptr);
        EXPECT_TRUE(result2->next());
        delete result2;
    }

    // 测试异常情况
    TEST_F(DbConnectionTest, ExceptionHandling)
    {
        DbConnection conn(host, user, password, database);

        // 测试无效SQL
        EXPECT_THROW({
                         conn.execute_query("INVALID SQL");
                     }, DBException);

        // 测试无效表名
        EXPECT_THROW({
                         conn.execute_query("SELECT * FROM non_existent_table");
                     }, DBException);

        // 测试无效where条件
        EXPECT_THROW({
                         conn.execute_query("SELECT * FROM test_table WHERE invalid_column = ?", std::string("value"));
                     }, DBException);
    }


}