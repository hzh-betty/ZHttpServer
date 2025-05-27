#include "db_connection.h"
#include <queue>
#include <condition_variable>
#include <thread>

namespace zhttp::zdb
{
    class DbConnectionPool
    {
    public:
        // 禁止拷贝
        DbConnectionPool(const DbConnectionPool &) = delete;

        DbConnectionPool &operator=(const DbConnectionPool &) = delete;

        static DbConnectionPool &get_instance()
        {
            static DbConnectionPool instance;
            return instance;
        }

        // 初始化连接池
        void init(const std::string &host, const std::string &user,
                  const std::string &password, const std::string &database,uint32_t pool_size = 10);

        // 获取连接
        std::shared_ptr<DbConnection> get_connection();

    private:
        // 构造函数
        DbConnectionPool();

        ~DbConnectionPool();

        // 获取连接
        std::shared_ptr<DbConnection> create_connection();

        //  检查连接
        void check_connections();
    private:
        std::string host_;
        std::string user_;
        std::string password_;
        std::string database_;
        std::queue<std::shared_ptr<DbConnection>> connections_;
        std::mutex mutex_;
        std::condition_variable cv_;
        bool initialized_ = false;
        std::thread check_thread_; // 添加检查线程
    };
} // namespace zhttp::zdb
