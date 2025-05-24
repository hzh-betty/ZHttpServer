#pragma once

#include <gtest/gtest.h>
#include "../../include/router/router.h"

namespace zhttp
{
    namespace zrouter
    {

        class TestHandler : public RouterHandler
        {
        public:
            void handle_request(const HttpRequest &req, HttpResponse *resp) override
            {
                resp->set_status_code(HttpResponse::StatusCode::OK);
                resp->set_body("handled:" + req.get_path());
            }
        };

        TEST(RouterTest, RegisterAndRouteHandler)
        {
            Router router;
            auto handler = std::make_shared<TestHandler>();
            router.register_handler("/foo", HttpRequest::Method::GET, handler);

            HttpRequest req;
            req.set_method(HttpRequest::Method::GET);
            req.set_path("/foo");
            HttpResponse resp;
            bool routed = router.route(req, &resp);

            EXPECT_TRUE(routed);
            EXPECT_EQ(resp.get_status_code(), HttpResponse::StatusCode::OK);
            EXPECT_EQ(resp.get_body(), "handled:/foo");
        }

        TEST(RouterTest, RegisterAndRouteCallback)
        {
            Router router;
            router.register_callback("/bar", HttpRequest::Method::POST,
                                     [](const HttpRequest &req, HttpResponse *resp)
                                     {
                                         resp->set_status_code(HttpResponse::StatusCode::Created);
                                         resp->set_body("cb:" + req.get_path());
                                     });

            HttpRequest req;
            req.set_method(HttpRequest::Method::POST);
            req.set_path("/bar");
            HttpResponse resp;
            bool routed = router.route(req, &resp);

            EXPECT_TRUE(routed);
            EXPECT_EQ(resp.get_status_code(), HttpResponse::StatusCode::Created);
            EXPECT_EQ(resp.get_body(), "cb:/bar");
        }

        TEST(RouterTest, RegexHandlerRouteAndParam)
        {
            Router router;
            auto handler = std::make_shared<TestHandler>();
            router.register_regex_handler("/user/:id", HttpRequest::Method::GET, handler);

            HttpRequest req;
            req.set_method(HttpRequest::Method::GET);
            req.set_path("/user/42");
            HttpResponse resp;
            bool routed = router.route(req, &resp);

            EXPECT_TRUE(routed);
            EXPECT_EQ(resp.get_status_code(), HttpResponse::StatusCode::OK);
            EXPECT_EQ(resp.get_body(), "handled:/user/42");
            // 路径参数 param1 应为 42
            // 原始 req 未变
            EXPECT_EQ(req.get_path_parameters("param1"), "");
        }

        TEST(RouterTest, RegexCallbackRouteAndParam)
        {
            Router router;
            router.register_regex_callback("/api/:type/:id", HttpRequest::Method::GET,
                                           [](const HttpRequest &req, HttpResponse *resp)
                                           {
                                               resp->set_status_code(HttpResponse::StatusCode::OK);
                                               resp->set_body(req.get_path_parameters("param1") + "-" +
                                                              req.get_path_parameters("param2"));
                                           });

            HttpRequest req;
            req.set_method(HttpRequest::Method::GET);
            req.set_path("/api/book/99");
            HttpResponse resp;
            bool routed = router.route(req, &resp);

            EXPECT_TRUE(routed);
            EXPECT_EQ(resp.get_status_code(), HttpResponse::StatusCode::OK);
            // param1=book, param2=99
            EXPECT_EQ(resp.get_body(), "book-99");
        }

        TEST(RouterTest, NotFoundRoute)
        {
            Router router;
            HttpRequest req;
            req.set_method(HttpRequest::Method::GET);
            req.set_path("/notfound");
            HttpResponse resp;
            bool routed = router.route(req, &resp);

            EXPECT_FALSE(routed);
        }

        TEST(RouterTest, MethodMismatch)
        {
            Router router;
            auto handler = std::make_shared<TestHandler>();
            router.register_handler("/foo", HttpRequest::Method::POST, handler);

            HttpRequest req;
            req.set_method(HttpRequest::Method::GET);
            req.set_path("/foo");
            HttpResponse resp;
            bool routed = router.route(req, &resp);

            EXPECT_FALSE(routed);
        }

    } // namespace zrouter
} // namespace zhttp