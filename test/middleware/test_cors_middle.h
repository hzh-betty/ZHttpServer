#pragma once

#include "../../include/middleware/cors/cors_middle.h"
#include <gtest/gtest.h>

namespace zhttp::zmiddleware
{
    // 模拟中间件调用后抛出 HttpResponse 的工具函数
    bool handle_preflight(CorsMiddleware &middleware, HttpRequest &request, HttpResponse &out_response)
    {
        try
        {
            middleware.before(request);
        } catch (const HttpResponse &resp)
        {
            out_response = resp;
            return true;
        }
        return false;
    }

    class CorsMiddlewareTest : public ::testing::Test
    {
    protected:
        CorsConfig config;
        CorsMiddleware *middleware{};

        void SetUp() override
        {
            config.allow_origins_ = {"https://example.com"};
            config.allow_methods_ = {"GET", "POST"};
            config.allow_headers_ = {"Content-Type", "Authorization"};
            config.server_origin_ = "https://api.server.com";
            config.allow_credentials = true;
            config.max_age = 600;

            middleware = new CorsMiddleware(config);
        }

        void TearDown() override
        {
            delete middleware;
        }

        static HttpRequest create_preflight_request(const std::string &origin)
        {
            HttpRequest req;
            req.set_method(HttpRequest::Method::OPTIONS);
            req.set_version("HTTP/1.1");
            req.set_header("Origin", origin);
            req.set_header("Access-Control-Request-Method", "POST");
            req.set_header("Access-Control-Request-Headers", "Authorization");
            return req;
        }

        static HttpRequest create_normal_request(const std::string &origin)
        {
            HttpRequest req;
            req.set_method(HttpRequest::Method::GET);
            req.set_version("HTTP/1.1");
            req.set_header("Origin", origin);
            return req;
        }
    };

    // 测试：非跨域请求不应该添加 CORS 头
    TEST_F(CorsMiddlewareTest, SameOriginShouldNotModifyResponse)
    {
        HttpRequest req = create_normal_request(config.server_origin_);
        HttpResponse res;

        middleware->before(req);
        middleware->after(res);

        EXPECT_TRUE(res.get_header("Access-Control-Allow-Origin").empty());
    }

    // 测试：合法跨域预检请求应正确响应并添加CORS头
    TEST_F(CorsMiddlewareTest, PreflightRequestShouldReturnProperHeaders)
    {
        HttpRequest req = create_preflight_request("https://example.com");
        HttpResponse res;

        bool caught = handle_preflight(*middleware, req, res);

        EXPECT_TRUE(caught);
        EXPECT_EQ(res.get_status_code(), HttpResponse::StatusCode::NoContent);
        EXPECT_EQ(res.get_header("Access-Control-Allow-Origin"), "https://example.com");
        EXPECT_EQ(res.get_header("Access-Control-Allow-Methods"), "GET,POST");
        EXPECT_EQ(res.get_header("Access-Control-Allow-Headers"), "Content-Type,Authorization");
        EXPECT_EQ(res.get_header("Access-Control-Max-Age"), "600");
        EXPECT_EQ(res.get_header("Access-Control-Allow-Credentials"), "true");
    }

    // 测试：非法源请求应被禁止
    TEST_F(CorsMiddlewareTest, DisallowedOriginShouldReturn403)
    {
        HttpRequest req = create_preflight_request("https://evil.com");
        HttpResponse res;

        bool caught = handle_preflight(*middleware, req, res);

        EXPECT_TRUE(caught);
        EXPECT_EQ(res.get_status_code(), HttpResponse::StatusCode::Forbidden);
    }

    // 测试：正常跨域 GET 请求应在 after 阶段添加头
    TEST_F(CorsMiddlewareTest, AfterShouldAddCORSHeadersIfOriginAllowed)
    {
        HttpRequest req = create_normal_request("https://example.com");
        HttpResponse res;

        middleware->before(req);
        middleware->after(res);

        EXPECT_EQ(res.get_header("Access-Control-Allow-Origin"), "https://example.com");
        EXPECT_EQ(res.get_header("Access-Control-Allow-Methods"), "GET,POST");
        EXPECT_EQ(res.get_header("Access-Control-Allow-Headers"), "Content-Type,Authorization");
        EXPECT_EQ(res.get_header("Access-Control-Allow-Credentials"), "true");
    }

    // 测试：当 allow_origins 为 * 时，应返回通配符
    TEST(CorsMiddlewareDefaultTest, WildcardOriginShouldReturnStar)
    {
        CorsConfig config = CorsConfig::default_config();
        CorsMiddleware middleware(config);

        HttpRequest req;
        req.set_method(HttpRequest::Method::GET);
        req.set_version("HTTP/1.1");
        req.set_header("Origin", "https://anyorigin.com");

        HttpResponse res;
        middleware.before(req);
        middleware.after(res);

        EXPECT_EQ(res.get_header("Access-Control-Allow-Origin"), "*");
    }

} // namespace zhttp
