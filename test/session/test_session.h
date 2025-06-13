#pragma once
#include "session/session.h"
#include <gtest/gtest.h>
#include <thread>
namespace zhttp
{
    namespace zsession
    {
        TEST(SessionTest, AttributeSetAndGet)
        {
            Session s("sid123");
            s.set_attribute("user", "alice");
            EXPECT_EQ(s.get_attribute("user"), "alice");
            EXPECT_EQ(s.get_attribute("not_exist"), "");
        }

        TEST(SessionTest, RemoveAndClearAttributes)
        {
            Session s("sid456");
            s.set_attribute("a", "1");
            s.set_attribute("b", "2");
            s.remove_attribute("a");
            EXPECT_EQ(s.get_attribute("a"), "");
            s.clear_attributes();
            EXPECT_EQ(s.get_attribute("b"), "");
        }

        TEST(SessionTest, ExpiryAndRefresh)
        {
            Session s("sid789", 1); // 1秒超时
            EXPECT_FALSE(s.is_expired());
            std::this_thread::sleep_for(std::chrono::seconds(2));
            EXPECT_TRUE(s.is_expired());
            s.refresh();
            EXPECT_FALSE(s.is_expired());
        }

        TEST(SessionTest, GetSessionId)
        {
            Session s("mysid");
            EXPECT_EQ(s.get_session_id(), "mysid");
        }
    } // namespace zsession
} // namespace zhttp