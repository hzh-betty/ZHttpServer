#pragma  once

#include "../../include/middleware/middleware_chain.h"
#include <gtest/gtest.h>
#include <memory>

namespace zhttp::zmiddleware
{
    class TrackingMiddleware : public Middleware
    {
    public:
        static std::vector<int> call_order; // 记录调用顺序
        int id; // 中间件标识符

        explicit TrackingMiddleware(int id) : id(id) {}

        void before(HttpRequest &) override
        {
            call_order.push_back(id);
        }

        void after(HttpResponse &) override
        {
            call_order.push_back(id * -1); // 负数表示 after 调用
        }
    };

    std::vector<int> TrackingMiddleware::call_order;

    class ExceptionMiddleware : public Middleware
    {
    public:
        void before(HttpRequest &) override {}

        void after(HttpResponse &) override
        {
            throw std::runtime_error("Test exception");
        }
    };


    TEST(MiddlewareChainTest, ProcessBeforeOrder)
    {
        MiddlewareChain chain;
        TrackingMiddleware::call_order.clear();

        // 创建三个可追踪中间件
        auto m1 = std::make_shared<TrackingMiddleware>(1);
        auto m2 = std::make_shared<TrackingMiddleware>(2);
        auto m3 = std::make_shared<TrackingMiddleware>(3);

        chain.add_middleware(m1);
        chain.add_middleware(m2);
        chain.add_middleware(m3);

        HttpRequest req;
        chain.process_before(req);

        // 验证 before 调用顺序：1 -> 2 -> 3
        ASSERT_EQ(TrackingMiddleware::call_order.size(), 3);
        EXPECT_EQ(TrackingMiddleware::call_order[0], 1);
        EXPECT_EQ(TrackingMiddleware::call_order[1], 2);
        EXPECT_EQ(TrackingMiddleware::call_order[2], 3);
    }

    TEST(MiddlewareChainTest, ProcessAfterReverseOrder)
    {
        MiddlewareChain chain;
        TrackingMiddleware::call_order.clear();

        auto m1 = std::make_shared<TrackingMiddleware>(1);
        auto m2 = std::make_shared<TrackingMiddleware>(2);

        chain.add_middleware(m1);
        chain.add_middleware(m2);

        HttpResponse res;
        chain.process_after(res);

        // 验证 after 调用顺序：2 -> 1 (用负数表示)
        ASSERT_EQ(TrackingMiddleware::call_order.size(), 2);
        EXPECT_EQ(TrackingMiddleware::call_order[0], -2);
        EXPECT_EQ(TrackingMiddleware::call_order[1], -1);
    }

    TEST(MiddlewareChainTest, HandleExceptionInAfter)
    {
        MiddlewareChain chain;
        TrackingMiddleware::call_order.clear();

        auto normal = std::make_shared<TrackingMiddleware>(1);
        auto exception = std::make_shared<ExceptionMiddleware>();
        auto after_exception = std::make_shared<TrackingMiddleware>(2);

        chain.add_middleware(normal);
        chain.add_middleware(exception);
        chain.add_middleware(after_exception);

        HttpResponse res;
        EXPECT_NO_THROW(chain.process_after(res));

        // 验证异常后仍继续执行后续中间件
        ASSERT_EQ(TrackingMiddleware::call_order.size(), 2);
        EXPECT_EQ(TrackingMiddleware::call_order[0], -2); // 异常中间件之后的中间件
        EXPECT_EQ(TrackingMiddleware::call_order[1], -1); // 正常中间件
    }

    TEST(MiddlewareChainTest, EmptyChainHandling)
    {
        MiddlewareChain chain;

        HttpRequest req;
        HttpResponse res;

        EXPECT_NO_THROW(chain.process_before(req));
        EXPECT_NO_THROW(chain.process_after(res));
    }
} // namespace zhttp::zmiddleware