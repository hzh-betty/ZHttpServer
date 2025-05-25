#include "../../include/session/session_manager.h"


namespace zhttp::zsession
{
    // 从请求中获取或创建会话，也就是说，如果请求中包含会话ID，则从存储中加载会话，否则创建一个新的会话
    std::shared_ptr<Session> SessionManager::get_session(const HttpRequest &request, HttpResponse *response)
    {
        std::string session_id = get_session_id_from_request(request);
        std::shared_ptr<Session> session;
        if (!session_id.empty())
        {
            session = session_storage_->load(session_id);
            // 如果会话存在，则检查是否过期
            if (session)
            {
                session->refresh();
                if (session->is_expired())
                {
                    // 如果会话已过期，则删除会话
                    session_storage_->remove(session_id);
                    session.reset();
                }
            }
        }

        if (!session)
        {
            // 如果会话不存在或已过期，则创建一个新的会话
            session_id = generate_session_id();
            session = std::make_shared<Session>(session_id);
            set_session_id_to_response(response, session_id);
            session_storage_->store(session);
        }

        return session;
    }

    // 设置会话存储
    void SessionManager::set_session_storage(std::unique_ptr<Storage> &&session_storage)
    {
        session_storage_ = std::move(session_storage);
    }


    // 销毁会话
    void SessionManager::destroy_session(const std::string &session_id)
    {
        session_storage_->remove(session_id);
    }

    // 更新会话
    void SessionManager::update_session(const std::shared_ptr<Session> &session)
    {
        session_storage_->store(session);
    }

    // 清理所有过期会话
    void SessionManager::cleanup_expired_sessions()
    {
        session_storage_->clear_expired();
    }

    // 生成随机会话ID
    std::string SessionManager::generate_session_id()
    {
        std::stringstream ss{};
        std::uniform_int_distribution<> dist(0, 15);
        // 生成32个字符的会话ID，每个字符是一个十六进制数字
        for (int i = 0; i < 32; ++i)
        {
            ss << std::hex << dist(rng_);
        }
        return ss.str();
    }

    // 从请求中获取会话ID
    std::string SessionManager::get_session_id_from_request(const HttpRequest &request)
    {
        std::string session_id;
        auto cookie = request.get_header("Cookie");
        if (!cookie.empty())
        {
            size_t pos = cookie.find("session_id=");
            if (pos != std::string::npos)
            {
                size_t end = cookie.find(';', pos);
                session_id = cookie.substr(pos + 11, end - pos - 11);
            }
        }
        return session_id;
    }

    // 设置会话ID到响应中Cookie
    void SessionManager::set_session_id_to_response(HttpResponse *response, const std::string &session_id)
    {
        std::string cookie = "session_id=" + session_id + "; Path=/; HttpOnly";
        response->set_header("Set-Cookie", cookie);
    }

} // namespace zhttp::zsession

// namespace