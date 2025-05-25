#pragma once

#include "session.h"
#include "session_storage.h"
#include "../http/http_request.h"
#include "../http/http_response.h"
#include <random>
#include <sstream>

namespace zhttp::zsession
{
    class SessionManager
    {
    public:
        SessionManager(const SessionManager &) = delete;

        SessionManager &operator=(const SessionManager &) = delete;

        // 获取单例实例
        static SessionManager &get_instance()
        {
            static SessionManager instance;
            return instance;
        }

        // 从请求中获取或创建会话，也就是说，如果请求中包含会话ID，则从存储中加载会话，否则创建一个新的会话
        std::shared_ptr<Session> get_session(const HttpRequest &request, HttpResponse *response);

        // 设置会话存储
        void set_session_storage(std::unique_ptr<Storage> &&session_storage);

        // 销毁会话
        void destroy_session(const std::string &session_id);

        // 更新会话
        void update_session(const std::shared_ptr<Session> &session);

        // 清除所有过期会话
        void cleanup_expired_sessions();

    private:
        SessionManager()
                : session_storage_(std::make_unique<InMemoryStorage>())
        {
        }


    private:
        // 生成随机会话ID
        std::string generate_session_id();

        // 从请求中获取会话ID
        static std::string get_session_id_from_request(const HttpRequest &request);

        // 设置会话ID到响应中Cookie
        static void set_session_id_to_response(HttpResponse *response, const std::string &session_id);

    private:
        std::unique_ptr<Storage> session_storage_; // 会话存储
        std::mt19937 rng_ = std::mt19937(std::random_device{}()); // 随机数生成器
    };
} // namespace zhttp::zsession

