#include <utility>
#include "../../include/db_pool/mysql_connection.h"
#include "../../include/log/logger.h"
#include"../../include/db_pool/db_exception.h"

namespace zhttp::zdb
{
    MysqlConnection::MysqlConnection(std::string host, std::string user,
                                     std::string password, std::string database)
        : host_(std::move(host)), user_(std::move(user)), password_(std::move(password)), database_(std::move(database))
    {
        ZHTTP_LOG_INFO("Creating database connection to {}@{}/{}", user_, host_, database_);
        try
        {
            std::lock_guard<std::mutex> lockGuard(mutex_);
            connect_helper();
            ZHTTP_LOG_INFO("Database connection created successfully");
        }
        catch (const sql::SQLException &e)
        {
            ZHTTP_LOG_ERROR("Failed to create database connection: {}", e.what());
            throw DBException(e.what());
        }
    }

    MysqlConnection::~MysqlConnection()
    {
        ZHTTP_LOG_DEBUG("Destroying database connection");
        cleanup();
        ZHTTP_LOG_INFO("Database connection closed");
    }

    // 检查连接是否可用
    bool MysqlConnection::ping() const
    {
        ZHTTP_LOG_DEBUG("Pinging database connection");
        try
        {
            std::lock_guard<std::mutex> lockGuard(mutex_);
            if (!connection_)
            {
                ZHTTP_LOG_WARN("Connection is null, ping failed");
                return false;
            }

            // 不使用 getStmt，直接创建新的语句
            std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
            std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery("SELECT 1"));
            while (rs && rs->next())
            {
                // 获取结果
            }
            ZHTTP_LOG_DEBUG("Database ping successful");
            return true;
        }
        catch (const sql::SQLException &e)
        {
            ZHTTP_LOG_ERROR("Database ping failed: {}", e.what());
            return false;
        }
    }

    // 检查连接是否可用
    bool MysqlConnection::is_valid() const
    {
        ZHTTP_LOG_DEBUG("Validating database connection");
        try
        {
            std::lock_guard<std::mutex> lockGuard(mutex_);

            if (!connection_)
            {
                ZHTTP_LOG_WARN("Connection is null, validation failed");
                return false;
            }

            std::unique_ptr<sql::Statement> stmt(connection_->createStatement());

            // 如果它返回了 resultset，就手动去取：
            if (stmt->execute("SELECT 1"))
            {
                const std::unique_ptr<sql::ResultSet> rs(stmt->getResultSet());
                while (rs && rs->next())
                {
                    // 获取结果
                }
            }

            ZHTTP_LOG_DEBUG("Database connection validation successful");
            return true;
        }
        catch (const sql::SQLException &e)
        {
            ZHTTP_LOG_WARN("Database connection validation failed: {}", e.what());
            return false;
        }
    }

    // 重连
    void MysqlConnection::reconnect()
    {
        ZHTTP_LOG_INFO("Attempting to reconnect to database {}@{}/{}", user_, host_, database_);
        try
        {
            std::lock_guard<std::mutex> lockGuard(mutex_);

            // 释放旧的连接
            connection_.reset();
            ZHTTP_LOG_DEBUG("Old connection released");

            connect_helper();
            ZHTTP_LOG_INFO("Database reconnection successful");
        }
        catch (const sql::SQLException &e)
        {
            ZHTTP_LOG_ERROR("Database reconnect failed: {}", e.what());
            throw DBException(e.what());
        }
    }

    // 清理连接
    void MysqlConnection::cleanup()
    {
        ZHTTP_LOG_DEBUG("Cleaning up database connection");
        try
        {
            std::lock_guard<std::mutex> lockGuard(mutex_);

            if (connection_)
            {
                // 确保所有事务都已完成
                if (!connection_->getAutoCommit())
                {
                    ZHTTP_LOG_DEBUG("Rolling back uncommitted transaction");
                    connection_->rollback();
                    connection_->setAutoCommit(true);
                }

                // 清理所有未处理的结果集
                std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
                int result_count = 0;
                while (stmt->getMoreResults())
                {
                    auto result = stmt->getResultSet();
                    while (result && result->next())
                    {
                        // 消费所有结果
                    }
                    result_count++;
                }

                if (result_count > 0)
                {
                    ZHTTP_LOG_DEBUG("Cleaned up {} pending result sets", result_count);
                }

                ZHTTP_LOG_DEBUG("Database connection cleanup completed");
            }
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_WARN("Error cleaning up connection: {}", e.what());
            try
            {
                ZHTTP_LOG_DEBUG("Attempting reconnect during cleanup");
                reconnect();
            }
            catch (const std::exception &reconnect_e)
            {
                ZHTTP_LOG_ERROR("Failed to reconnect during cleanup: {}", reconnect_e.what());
            }
        }
    }

    // 辅助连接函数
    void MysqlConnection::connect_helper()
    {
        ZHTTP_LOG_DEBUG("Establishing database connection");

        // 获取数据库驱动
        sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
        ZHTTP_LOG_DEBUG("MySQL driver instance obtained");

        // 创建数据库连接
        connection_.reset(driver->connect(host_, user_, password_));
        ZHTTP_LOG_DEBUG("Database connection established to {}@{}", user_, host_);

        if (connection_)
        {
            // 设置数据库
            connection_->setSchema(database_);
            ZHTTP_LOG_DEBUG("Database schema set to: {}", database_);

            // 配置数据库连接
            connection_->setClientOption("OPT_CONNECT_TIMEOUT", "10");
            connection_->setClientOption("multi_statements", "false");
            ZHTTP_LOG_DEBUG("Database connection options configured");

            // 设置字符集为 utf8mb4，确保支持 Emoji 等多字节字符
            std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
            stmt->execute("SET NAMES utf8mb4");
            ZHTTP_LOG_DEBUG("Character set configured to utf8mb4");

            ZHTTP_LOG_INFO("Database connection fully established and configured");
        }
        else
        {
            ZHTTP_LOG_ERROR("Failed to create database connection object");
            throw DBException("Failed to create database connection");
        }
    }
} // namespace zhttp::zdb
