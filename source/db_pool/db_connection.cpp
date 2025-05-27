#include "../../include/db_pool/db_connection.h"

namespace zhttp::zdb
{
    DbConnection::DbConnection(const std::string &host, const std::string &user,
                               const std::string &password, const std::string &database)
            : host_(host), user_(user), password_(password), database_(database)
    {
        try
        {
            // 获取数据库驱动
            sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();

            // 创建数据库连接
            connection_.reset(driver->connect(host, user, password));

            LOG_DEBUG << "Successfully created database connection to " << host << ":" << user << "@" << database;

            if (connection_)
            {
                // 设置数据库
                connection_->setSchema(database);

                // 配置数据库连接
                connection_->setClientOption("OPT_CONNECT_TIMEOUT", "10");
                connection_->setClientOption("OPT_READ_TIMEOUT", "30");
                connection_->setClientOption("multi_statements", "false");

                // 设置字符集为 utf8mb4，确保支持 Emoji 等多字节字符
                std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
                stmt->execute("SET NAMES utf8mb4");
                LOG_INFO << "Database connection established";
            }

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
    bool DbConnection::ping()
    {
        try
        {
            if (!connection_) return false;

            // 不使用 getStmt，直接创建新的语句
            std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
            std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery("SELECT 1"));
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
            if (!connection_) return false;
            std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
            stmt->execute("SELECT 1");
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
            if (connection_)
            {
                connection_->reconnect();
            }
            else
            {
                sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
                connection_.reset(driver->connect(host_, user_, password_));
                connection_->setSchema(database_);
            }
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
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
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

}// namespace zhttp::zdb
