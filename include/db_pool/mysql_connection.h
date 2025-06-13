#pragma once

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <mysql_driver.h>
#include <mysql/mysql.h>
#include "../log/logger.h"
#include "db_exception.h"

namespace zhttp::zdb
{
    // 一次性拉取所有查询结果：行列表，每行是 string 列表
    using QueryResult = std::vector<std::vector<std::string>>;

    class MysqlConnection
    {
    public:
        MysqlConnection(std::string host, std::string user,
                     std::string password, std::string database);

        ~MysqlConnection();

        // 禁止拷贝与赋值
        MysqlConnection(const MysqlConnection &) = delete;

        MysqlConnection &operator=(const MysqlConnection &) = delete;

        // 判断连接是否有效
        [[nodiscard]] bool is_valid() const;

        // 重新连接
        void reconnect();

        // 清理
        void cleanup();

        // 执行sql语句
        template<typename ...Args>
        QueryResult execute_query(const std::string &sql, Args &&... args)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            try
            {
                // 1. 准备语句并绑定参数
                std::unique_ptr<sql::PreparedStatement> stmt(
                        connection_->prepareStatement(sql)
                );
                bind_params(stmt.get(), 1, std::forward<Args>(args)...);

                // 2. 执行查询
                std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery());

                // 3. 获取列数信息该库通过 ResultSet::getMetaData() 返回的元数据对象指针，
                // 其生命周期由 ResultSet 内部管理，用户不应手动释放。
                sql::ResultSetMetaData* meta(rs->getMetaData());
                uint32_t colCount = meta->getColumnCount();

                // 4. 一次性消费所有行
                QueryResult rows;
                while (rs->next())
                {
                    std::vector<std::string> row;
                    row.reserve(colCount);
                    for (int i = 1; i <= colCount; ++i)
                    {
                        row.emplace_back(rs->getString(i));
                    }
                    rows.emplace_back(std::move(row));
                }

                // 5. stmt 和 rs 出作用域自动析构，底层游标被关闭
                return rows;
            }
            catch (sql::SQLException &e)
            {
                ZHTTP_LOG_ERROR("execute_query error: {}", e.what());
                throw DBException(e.what());
            }
        }

        // 执行更新
        template<typename ...Args>
        int execute_update(const std::string &sql, Args &&... args)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            try
            {
                // 直接创建新的预处理语句，不缓存
                std::unique_ptr<sql::PreparedStatement> statement(connection_->prepareStatement(sql));
                bind_params(statement.get(), 1, std::forward<Args>(args)...);
                int ret = statement->executeUpdate();
                return ret;
            }
            catch (sql::SQLException &e)
            {
                ZHTTP_LOG_ERROR("execute_update error: {}", e.what());
                throw DBException(e.what());
            }
        }

        // 添加检测连接是否有效
        bool ping() const;

    private:
        // 辅助连接并配置
        void connect_helper();

        // 辅助递归结束函数
        void bind_params(sql::PreparedStatement *, int) {}

        template<typename T, typename ...Args>
        void bind_params(sql::PreparedStatement *statement, int index, T &&value, Args &&... args)
        {
            if constexpr (std::is_same_v<std::decay_t<T>, std::string> || std::is_same_v<std::decay_t<T>, const char *>)
            {
                statement->setString(index, std::forward<T>(value));
            }
            else if constexpr (std::is_arithmetic_v<std::decay_t<T>>)
            {
                statement->setString(index, std::to_string(std::forward<T>(value)));
            }
            else
            {
                static_assert(false, "Unsupported parameter type");
            }
            bind_params(statement, index + 1, std::forward<Args>(args)...);
        }

    private:
        std::shared_ptr<sql::Connection> connection_; // 数据库连接
        std::string host_;//  数据库主机
        std::string user_; // 数据库用户名
        std::string password_; // 数据库密码
        std::string database_; // 数据库名
        mutable std::mutex mutex_;
    };
} // namespace zhttp::zdb

