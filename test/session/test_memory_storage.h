#pragma once
#include "../../include/session/memory_storage.h"
#include <thread>
#include <gtest/gtest.h>

namespace zhttp::zsession
{
    TEST(MemoryStorageTest, StoreAndLoad)
    {
        InMemoryStorage storage;
        auto session = std::make_shared<Session>("sid1");
        session->set_attribute("k", "v");
        storage.store(session);

        auto loaded = storage.load("sid1");
        ASSERT_NE(loaded, nullptr);
        EXPECT_EQ(loaded->get_attribute("k"), "v");
    }

    TEST(MemoryStorageTest, RemoveSession)
    {
        InMemoryStorage storage;
        auto session = std::make_shared<Session>("sid2");
        storage.store(session);
        storage.remove("sid2");
        EXPECT_EQ(storage.load("sid2"), nullptr);
    }

    TEST(MemoryStorageTest, ClearExpired)
    {
        InMemoryStorage storage;
        auto session = std::make_shared<Session>("sid3", 1);
        storage.store(session);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        storage.clear_expired();
        EXPECT_EQ(storage.load("sid3"), nullptr);
    }
} // namespace zhttp::zsession
