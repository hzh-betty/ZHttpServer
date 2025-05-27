#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <mysql_driver.h>
#include <mysql/mysql.h>
#include <muduo/base/Logging.h>
#include "db_exception.h"

namespace zhttp::zdb
{
    class DbConnection
    {
    public:
        DbConnection(const std::string &host, const std::string &user,
                     const std::string &password, const std::string &database);

        ~DbConnection();

        // 禁止拷贝与赋值
        DbConnection(const DbConnection &) = delete;

        DbConnection &operator=(const DbConnection &) = delete;

        // 判断连接是否有效
        bool is_valid() const;

        // 重新连接
        void reconnect();

        // 清理
        void cleanup();

        template<typename ...Args>
        sql::ResultSet *execute_query(const std::string &sql, Args &&... args)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            try
            {
                // 直接创建新的预处理语句，不缓存
                std::unique_ptr<sql::PreparedStatement> statement(connection_->prepareStatement(sql));
                bind_params(statement.get(), 1, std::forward<Args>(args)...);
                sql::ResultSet* res = statement->executeQuery();
                return res;
            }
            catch (sql::SQLException &e)
            {
                LOG_ERROR << "execute_query error: " << e.what();
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
                LOG_ERROR << "execute_update error: " << e.what();
                throw DBException(e.what());
            }
        }

        // 添加检测连接是否有效
        bool ping();

    private:

        // 辅助递归结束函数
        void bind_params(sql::PreparedStatement *, int) {}

        template<typename T, typename ...Args>
        void bind_params(sql::PreparedStatement *statement, int index, T &&value, Args &&... args)
        {
            if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
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
        std::mutex mutex_;
    };
} // namespace zhttp::zdb

