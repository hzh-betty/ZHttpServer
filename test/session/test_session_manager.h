#pragma once
#include "session/session_manager.h"
#include <gtest/gtest.h>
#include <thread>

namespace zhttp::zsession
{
    TEST(SessionManagerTest, CreateAndGetSession)
    {
        HttpRequest req;
        HttpResponse resp;
        auto &mgr = SessionManager::get_instance();
        auto session = mgr.get_session(req, &resp);
        EXPECT_NE(session, nullptr);
        EXPECT_FALSE(session->get_session_id().empty());
    }

    TEST(SessionManagerTest, SessionReuseWithCookie)
    {
        HttpRequest req;
        HttpResponse resp;
        auto &mgr = SessionManager::get_instance();
        auto session1 = mgr.get_session(req, &resp);
        std::string sid = session1->get_session_id();

        HttpRequest req2;
        req2.set_header("Cookie", "session_id=" + sid);
        HttpResponse resp2;
        auto session2 = mgr.get_session(req2, &resp2);
        EXPECT_EQ(session2->get_session_id(), sid);
    }

    TEST(SessionManagerTest, DestroySession)
    {
        HttpRequest req;
        HttpResponse resp;
        auto &mgr = SessionManager::get_instance();
        auto session = mgr.get_session(req, &resp);
        std::string sid = session->get_session_id();
        mgr.destroy_session(sid);

        HttpRequest req2;
        req2.set_header("Cookie", "session_id=" + sid);
        HttpResponse resp2;
        auto session2 = mgr.get_session(req2, &resp2);
        EXPECT_NE(session2->get_session_id(), sid); // 新会话
    }

    TEST(SessionManagerTest, CleanupExpiredSessions)
    {
        auto &mgr = SessionManager::get_instance();
        auto session = std::make_shared<Session>("exp1", 1);
        mgr.update_session(session);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        mgr.cleanup_expired_sessions();
        // 需要 Storage 层配合验证
    }
} // namespace zhttp::zsession
