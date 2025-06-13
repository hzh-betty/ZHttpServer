#pragma once

#include <gtest/gtest.h>
#include "http/http_context.h"
#include <muduo/net/Buffer.h>
#include <muduo/base/Timestamp.h>

namespace zhttp
{
    TEST(HttpContextTest, ParseEmptyRequest)
    {
        HttpContext ctx;
        muduo::net::Buffer buf;
        muduo::Timestamp now = muduo::Timestamp::now();

        bool ok = ctx.parse_request(&buf, now);
        EXPECT_FALSE(ok);
        EXPECT_FALSE(ctx.is_parse_complete());
    }

    TEST(HttpContextTest, ParseIncompleteRequestLine)
    {
        HttpContext ctx;
        muduo::net::Buffer buf;
        std::string req = "GET /api/test";
        buf.append(req);
        muduo::Timestamp now = muduo::Timestamp::now();

        bool ok = ctx.parse_request(&buf, now);
        EXPECT_FALSE(ok);
        EXPECT_FALSE(ctx.is_parse_complete());
    }

    TEST(HttpContextTest, ParsePostWithoutContentLength)
    {
        HttpContext ctx;
        muduo::net::Buffer buf;
        std::string req = "POST /submit HTTP/1.1\r\n\r\n";
        buf.append(req);
        muduo::Timestamp now = muduo::Timestamp::now();

        bool ok = ctx.parse_request(&buf, now);
        EXPECT_TRUE(ok);
        EXPECT_TRUE(ctx.is_parse_complete());
        EXPECT_EQ(ctx.request().get_method(), HttpRequest::Method::POST);
        EXPECT_EQ(ctx.request().get_path(), "/submit");
        EXPECT_EQ(ctx.request().get_version(), "HTTP/1.1");
        EXPECT_EQ(ctx.request().get_content(), ""); // 没有请求体
    }

    TEST(HttpContextTest, ParseWithExtraHeaders)
    {
        HttpContext ctx;
        muduo::net::Buffer buf;
        std::string req = "GET /api/test HTTP/1.1\r\nHost : localhost\r\nX-Extra-Header: value\r\n\r\n";
        buf.append(req);
        muduo::Timestamp now = muduo::Timestamp::now();

        bool ok = ctx.parse_request(&buf, now);
        EXPECT_TRUE(ok);
        EXPECT_TRUE(ctx.is_parse_complete());
        EXPECT_EQ(ctx.request().get_method(), HttpRequest::Method::GET);
        EXPECT_EQ(ctx.request().get_path(), "/api/test");
        EXPECT_EQ(ctx.request().get_version(), "HTTP/1.1");
        EXPECT_EQ(ctx.request().get_header("Host"), "localhost");
        EXPECT_EQ(ctx.request().get_header("X-Extra-Header"), "value");
        EXPECT_EQ(ctx.request().get_content(), ""); // 没有请求体
    }

    TEST(HttpContextTest, ParseWithExcessBody)
    {
        HttpContext ctx;
        muduo::net::Buffer buf;
        std::string req = "POST /submit HTTP/1.0\r\nContent-Length : 10\r\n\r\nhelloextra";
        buf.append(req);
        muduo::Timestamp now = muduo::Timestamp::now();

        bool ok = ctx.parse_request(&buf, now);
        EXPECT_TRUE(ok);
        EXPECT_TRUE(ctx.is_parse_complete());
        EXPECT_EQ(ctx.request().get_method(), HttpRequest::Method::POST);
        EXPECT_EQ(ctx.request().get_path(), "/submit");
        EXPECT_EQ(ctx.request().get_version(), "HTTP/1.0");
        EXPECT_EQ(ctx.request().get_content_length(), 10);
        EXPECT_EQ(ctx.request().get_content(), "helloextra");
    }

    TEST(HttpContextTest, ParseGetWithoutHeaders)
    {
        HttpContext ctx;
        muduo::net::Buffer buf;
        std::string req = "GET /api/test HTTP/1.1\r\n\r\n";
        buf.append(req);
        muduo::Timestamp now = muduo::Timestamp::now();

        bool ok = ctx.parse_request(&buf, now);
        EXPECT_TRUE(ok);
        EXPECT_TRUE(ctx.is_parse_complete());
        EXPECT_EQ(ctx.request().get_method(), HttpRequest::Method::GET);
        EXPECT_EQ(ctx.request().get_path(), "/api/test");
        EXPECT_EQ(ctx.request().get_version(), "HTTP/1.1");

    }

    TEST(HttpContextTest, ParseWithDuplicateHeaders)
    {
        HttpContext ctx;
        muduo::net::Buffer buf;
        std::string req = "GET /api/test HTTP/1.1\r\nHost: localhost\r\nHost: example.com\r\n\r\n";
        buf.append(req);
        muduo::Timestamp now = muduo::Timestamp::now();

        bool ok = ctx.parse_request(&buf, now);
        EXPECT_TRUE(ok);
        EXPECT_TRUE(ctx.is_parse_complete());
        EXPECT_EQ(ctx.request().get_header("Host"), "example.com"); // 后者覆盖前者
    }

    TEST(HttpContextTest, ParseSimpleGet)
    {
        HttpContext ctx;
        muduo::net::Buffer buf;
        std::string req = "GET /api/test?foo=bar&name=liming HTTP/1.1\r\nHost: localhost\r\nContent-Length: 11\r\n\r\nhello";
        buf.append(req);
        muduo::Timestamp now = muduo::Timestamp::now();

        bool ok = ctx.parse_request(&buf, now);
        EXPECT_TRUE(ok);
        EXPECT_FALSE(ctx.is_parse_complete()); // 还未解析body

        // 继续解析body
        buf.append(" world");
        ctx.parse_request(&buf, now);
        EXPECT_TRUE(ctx.is_parse_complete());

        EXPECT_EQ(ctx.request().get_method(), HttpRequest::Method::GET);
        EXPECT_EQ(ctx.request().get_path(), "/api/test");
        EXPECT_EQ(ctx.request().get_query_parameters("foo"), "bar");
        EXPECT_EQ(ctx.request().get_query_parameters("name"), "liming");
        EXPECT_EQ(ctx.request().get_version(), "HTTP/1.1");
        EXPECT_EQ(ctx.request().get_header("Content-Length"), "11");
        EXPECT_EQ(ctx.request().get_content(), "hello world");
    }

    TEST(HttpContextTest, ParseInvalidRequestLine)
    {
        HttpContext ctx;
        muduo::net::Buffer buf;
        std::string req = "BADMETHOD / HTTP/1.1\r\n\r\n";
        buf.append(req);
        muduo::Timestamp now = muduo::Timestamp::now();

        bool ok = ctx.parse_request(&buf, now);
        EXPECT_FALSE(ok);
    }

    TEST(HttpContextTest, ParseHeadersAndBody)
    {
        HttpContext ctx;
        muduo::net::Buffer buf;
        std::string req = "POST /submit HTTP/1.1\r\nContent-Length: 4\r\n\r\ndata";
        buf.append(req);
        muduo::Timestamp now = muduo::Timestamp::now();

        bool ok = ctx.parse_request(&buf, now);
        EXPECT_TRUE(ok);
        ctx.parse_request(&buf, now);
        EXPECT_TRUE(ctx.is_parse_complete());

        EXPECT_EQ(ctx.request().get_method(), HttpRequest::Method::POST);
        EXPECT_EQ(ctx.request().get_path(), "/submit");
        EXPECT_EQ(ctx.request().get_version(), "HTTP/1.1");
        EXPECT_EQ(ctx.request().get_header("Content-Length"), "4");
        EXPECT_EQ(ctx.request().get_content(), "data");
    }

} // namespace zhttp