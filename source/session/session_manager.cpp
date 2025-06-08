#include "../../include/session/session_manager.h"
#include "../../include/log/logger.h"
#include <sstream>
#include <iomanip>

namespace zhttp::zsession
{
    SessionManager::SessionManager() : session_storage_(std::make_unique<InMemoryStorage>())
    {
        ZHTTP_LOG_INFO("SessionManager initialized with default memory storage");
    }

    // 从请求中获取或创建会话
    std::shared_ptr<Session> SessionManager::get_session(const HttpRequest &request, HttpResponse *response)
    {
        ZHTTP_LOG_DEBUG("Getting session from request");
        
        std::shared_lock<std::shared_mutex> lock(rb_mutex_);
        
        std::string session_id = get_session_id_from_request(request);
        
        if (!session_id.empty())
        {
            ZHTTP_LOG_DEBUG("Found session ID in request: {}", session_id);
            
            // 尝试从存储中加载现有会话
            if (auto session = session_storage_->load(session_id))
            {
                if (!session->is_expired())
                {
                    session->refresh(); // 刷新会话过期时间
                    session_storage_->store(session); // 更新存储
                    ZHTTP_LOG_INFO("Existing session {} loaded and refreshed", session_id);
                    return session;
                }
                else
                {
                    ZHTTP_LOG_WARN("Session {} has expired, will create new session", session_id);
                    session_storage_->remove(session_id);
                }
            }
            else
            {
                ZHTTP_LOG_DEBUG("Session {} not found in storage", session_id);
            }
        }
        
        // 创建新会话
        session_id = generate_session_id();
        auto new_session = std::make_shared<Session>(session_id);
        session_storage_->store(new_session);
        set_session_id_to_response(response, session_id);
        
        ZHTTP_LOG_INFO("New session {} created and stored", session_id);
        return new_session;
    }

    // 设置会话存储
    void SessionManager::set_session_storage(std::unique_ptr<Storage> &&session_storage)
    {
        ZHTTP_LOG_INFO("Setting custom session storage");
        std::unique_lock<std::shared_mutex> lock(rb_mutex_);
        session_storage_ = std::move(session_storage);
        ZHTTP_LOG_INFO("Custom session storage updated successfully");
    }

    // 销毁会话
    void SessionManager::destroy_session(const std::string &session_id) const
    {
        ZHTTP_LOG_INFO("Destroying session: {}", session_id);
        
        std::shared_lock<std::shared_mutex> lock(rb_mutex_);
        session_storage_->remove(session_id);
        
        ZHTTP_LOG_INFO("Session {} destroyed successfully", session_id);
    }

    // 更新会话
    void SessionManager::update_session(const std::shared_ptr<Session> &session) const
    {
        ZHTTP_LOG_DEBUG("Updating session: {}", session->get_session_id());
        
        std::shared_lock<std::shared_mutex> lock(rb_mutex_);
        session_storage_->store(session);
        
        ZHTTP_LOG_DEBUG("Session {} updated successfully", session->get_session_id());
    }

    // 清理所有过期会话
    void SessionManager::cleanup_expired_sessions() const
    {
        ZHTTP_LOG_INFO("Starting cleanup of expired sessions");
        
        std::shared_lock<std::shared_mutex> lock(rb_mutex_);
        session_storage_->clear_expired();
        
        ZHTTP_LOG_INFO("Expired sessions cleanup completed");
    }

    // 生成随机会话ID
    std::string SessionManager::generate_session_id()
    {
        ZHTTP_LOG_DEBUG("Generating new session ID");
        
        std::uniform_int_distribution<> dis(0, 15);
        std::stringstream ss;
        
        for (int i = 0; i < 32; ++i)
        {
            ss << std::hex << dis(rng_);
        }
        
        std::string session_id = ss.str();
        ZHTTP_LOG_DEBUG("Generated session ID: {}", session_id);
        return session_id;
    }

    // 从请求中获取会话ID
    std::string SessionManager::get_session_id_from_request(const HttpRequest &request)
    {
        ZHTTP_LOG_DEBUG("Extracting session ID from request headers");
        
        std::string cookie_header = request.get_header("Cookie");
        if (cookie_header.empty())
        {
            ZHTTP_LOG_DEBUG("No Cookie header found in request");
            return "";
        }

        ZHTTP_LOG_DEBUG("Found Cookie header: {}", cookie_header);
        
        // 解析Cookie头，查找session_id
        size_t pos = cookie_header.find("session_id=");
        if (pos == std::string::npos)
        {
            ZHTTP_LOG_DEBUG("No session_id found in Cookie header");
            return "";
        }

        pos += 11; // "session_id="的长度
        size_t end_pos = cookie_header.find(';', pos);
        if (end_pos == std::string::npos)
        {
            end_pos = cookie_header.length();
        }

        std::string session_id = cookie_header.substr(pos, end_pos - pos);
        ZHTTP_LOG_DEBUG("Extracted session ID from Cookie: {}", session_id);
        return session_id;
    }

    // 设置会话ID到响应中Cookie
    void SessionManager::set_session_id_to_response(HttpResponse *response, const std::string &session_id)
    {
        ZHTTP_LOG_DEBUG("Setting session ID {} to response Cookie", session_id);
        
        std::string cookie_value = "session_id=" + session_id + "; Path=/; HttpOnly";
        response->set_header("Set-Cookie", cookie_value);
        
        ZHTTP_LOG_DEBUG("Session ID set in response Cookie successfully");
    }

} // namespace zhttp::zsession
