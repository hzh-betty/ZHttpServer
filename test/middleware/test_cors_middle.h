#pragma  once

#include "../../include/middleware/cors/cors_middle.h"
#include <gtest/gtest.h>

namespace zhttp::zmiddleware
{
    // 创建默认 GET 请求
    HttpRequest create_get_request(const std::string &origin = "")
    {
        HttpRequest request;
        request.set_method(HttpRequest::Method::GET);
        if (!origin.empty())
        {
            request.set_header("Origin", origin);
        }
        return request;
    }

    // 创建默认响应
    HttpResponse create_ok_response()
    {
        HttpResponse response;
        response.set_status_code(HttpResponse::StatusCode::OK);
        return response;
    }

    // 为测试 CorsMiddleware 提供访问 config_ 的方法
    class TestableCorsMiddleware : public CorsMiddleware
    {
    public:
        using CorsMiddleware::CorsMiddleware;

        const CorsConfig &get_cors_config() const { return config_; }
    };


    // 1. 构造函数与默认配置验证
    TEST(CorsMiddlewareTest, DefaultConstructorSetsDefaultConfig)
    {
        TestableCorsMiddleware middleware;

        const CorsConfig &config = middleware.get_cors_config();
        EXPECT_EQ(config.allow_origins_, std::vector<std::string>{"*"});
        EXPECT_EQ(config.allow_methods_, std::vector<std::string>({"GET", "POST", "PUT", "DELETE", "OPTIONS"}));
        EXPECT_EQ(config.allow_headers_, std::vector<std::string>({"Content-Type", "Authorization"}));
    }

    // 2. before 方法处理 OPTIONS 请求并抛出响应
    TEST(CorsMiddlewareTest, BeforeHandlesOptionsRequest)
    {
        TestableCorsMiddleware middleware;

        HttpRequest request = create_get_request("http://example.com");
        request.set_method(HttpRequest::Method::OPTIONS);

        EXPECT_THROW(middleware.before(request), HttpResponse);
    }

    // 3. after 方法添加 Access-Control-Allow-Origin 头部
    TEST(CorsMiddlewareTest, AfterAddsAllowOriginHeader)
    {
        CorsConfig config;
        config.allow_origins_ = {"http://allowed.com"};
        TestableCorsMiddleware middleware(config);

        HttpResponse response = create_ok_response();
        middleware.after(response);

        EXPECT_EQ(response.get_header("Access-Control-Allow-Origin"), "http://allowed.com");
    }

    // 4. join 方法拼接字符串向量
    TEST(CorsMiddlewareTest, JoinWorksCorrectly)
    {
        TestableCorsMiddleware middleware;

        std::vector<std::string> vec = {"A", "B", "C"};
        EXPECT_EQ(middleware.join(vec, ","), "A,B,C");
    }


    // 5. after 方法处理空 allow_origins_（允许所有源）
    TEST(CorsMiddlewareTest, AfterHandlesEmptyAllowOrigins)
    {
        CorsConfig config;
        config.allow_origins_.clear(); // 表示允许所有源
        TestableCorsMiddleware middleware(config);

        HttpResponse response = create_ok_response();
        middleware.after(response);

        EXPECT_EQ(response.get_header("Access-Control-Allow-Origin"), "*");
    }

    // 6. after 方法处理 allow_origins_ 包含 * 的情况
    TEST(CorsMiddlewareTest, AfterHandlesAllowAll)
    {
        CorsConfig config;
        config.allow_origins_ = {"*", "http://allowed.com"}; // * 在第一位
        TestableCorsMiddleware middleware(config);

        HttpResponse response = create_ok_response();
        middleware.after(response);

        EXPECT_EQ(response.get_header("Access-Control-Allow-Origin"), "*");
    }

    // 7. after 方法处理非通配符的多个源，取第一个
    TEST(CorsMiddlewareTest, AfterUsesFirstOriginIfNotWildCard)
    {
        CorsConfig config;
        config.allow_origins_ = {"http://a.com", "http://b.com"};
        TestableCorsMiddleware middleware(config);

        HttpResponse response = create_ok_response();
        middleware.after(response);

        EXPECT_EQ(response.get_header("Access-Control-Allow-Origin"), "http://a.com");
    }

} // namespace zhttp