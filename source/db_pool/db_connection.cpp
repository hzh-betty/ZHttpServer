#include <utility>
#include "../../include/db_pool/db_connection.h"

namespace zhttp::zdb
{
    DbConnection::DbConnection(std::string host, std::string user,
                               std::string password, std::string database)
            : host_(std::move(host)), user_(std::move(user)), password_(std::move(password)), database_(std::move(database))
    {
        try
        {
            std::lock_guard<std::mutex> lockGuard(mutex_);
            connect_helper();
        }
        catch (const sql::SQLException &e)
        {
            LOG_ERROR << "Failed to create database connection: " << e.what();
            throw DBException(e.what());
        }
    }

    DbConnection::~DbConnection()
    {
        cleanup();
        LOG_INFO << "Database connection closed";
    }

    // 检查连接是否可用
    bool DbConnection::ping() const
    {
        try
        {
            std::lock_guard<std::mutex> lockGuard(mutex_);
            if (!connection_) return false;

            // 不使用 getStmt，直接创建新的语句
            std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
            std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery("SELECT 1"));
            while (rs && rs->next())
            {
                // 获取结果
            }
            return true;
        }
        catch (const sql::SQLException &e)
        {
            LOG_ERROR << "Ping failed: " << e.what();
            return false;
        }
    }

    // 检查连接是否可用
    bool DbConnection::is_valid() const
    {
        try
        {
            std::lock_guard<std::mutex> lockGuard(mutex_);

            if (!connection_) return false;
            std::unique_ptr<sql::Statement> stmt(connection_->createStatement());

            // 如果它返回了 resultset，就手动去取：
            if (stmt->execute("SELECT 1"))
            {
                const std::unique_ptr<sql::ResultSet> rs (stmt->getResultSet());
                while (rs && rs->next())
                {
                    // 获取结果
                }
            }

            return true;
        }
        catch (const sql::SQLException &)
        {
            return false;
        }
    }

    // 重连
    void DbConnection::reconnect()
    {
        try
        {
            std::lock_guard<std::mutex> lockGuard(mutex_);

            // 释放旧的连接
            connection_.reset();

            connect_helper();

        }
        catch (const sql::SQLException &e)
        {
            LOG_ERROR << "Reconnect failed: " << e.what();
            throw DBException(e.what());
        }
    }

    // 清理连接
    void DbConnection::cleanup()
    {
        try
        {
            std::lock_guard<std::mutex> lockGuard(mutex_);

            if (connection_)
            {
                // 确保所有事务都已完成
                if (!connection_->getAutoCommit())
                {
                    connection_->rollback();
                    connection_->setAutoCommit(true);
                }

                // 清理所有未处理的结果集
                std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
                while (stmt->getMoreResults())
                {
                    auto result = stmt->getResultSet();
                    while (result && result->next())
                    {
                        // 消费所有结果
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            LOG_WARN << "Error cleaning up connection: " << e.what();
            try
            {
                reconnect();
            }
            catch (...)
            {
                // 忽略重连错误
            }
        }
    }

    // 辅助连接函数
    void DbConnection::connect_helper()
    {
        // 获取数据库驱动
        sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();

        // 创建数据库连接
        connection_.reset(driver->connect(host_, user_, password_));

        LOG_DEBUG << "Successfully created database connection to " << host_ << ":" << user_ << "@" << database_;

        if (connection_)
        {
            // 设置数据库
            connection_->setSchema(database_);

            // 配置数据库连接
            connection_->setClientOption("OPT_CONNECT_TIMEOUT", "10");
            connection_->setClientOption("multi_statements", "false");

            // 设置字符集为 utf8mb4，确保支持 Emoji 等多字节字符
            std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
            stmt->execute("SET NAMES utf8mb4");
            LOG_INFO << "Database connection established";
        }
    }

}// namespace zhttp::zdb
