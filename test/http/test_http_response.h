#include "../../include/http/http_response.h"
#include <gtest/gtest.h>
#include <muduo/net/Buffer.h>

namespace zhttp
{

    TEST(HttpResponseTest, Version) {
        HttpResponse resp;
        resp.set_version("HTTP/1.1");
        EXPECT_EQ(resp.get_version(), "HTTP/1.1");
    }

    TEST(HttpResponseTest, StatusCodeAndMessage) {
        HttpResponse resp;
        resp.set_status_code(HttpResponse::StatusCode::NotFound);
        resp.set_status_message("Not Found");
        EXPECT_EQ(resp.get_status_code(), HttpResponse::StatusCode::NotFound);
        EXPECT_EQ(resp.get_status_message(), "Not Found");
    }

    TEST(HttpResponseTest, SetResponseLine) {
        HttpResponse resp;
        resp.set_response_line("HTTP/1.0", HttpResponse::StatusCode::OK, "OK");
        EXPECT_EQ(resp.get_version(), "HTTP/1.0");
        EXPECT_EQ(resp.get_status_code(), HttpResponse::StatusCode::OK);
        EXPECT_EQ(resp.get_status_message(), "OK");
    }

    TEST(HttpResponseTest, Header) {
        HttpResponse resp;
        resp.set_header("Content-Type", "text/plain");
        EXPECT_EQ(resp.get_header("Content-Type"), "text/plain");
        EXPECT_EQ(resp.get_header("Not-Exist"), "");
    }

    TEST(HttpResponseTest, Body) {
        HttpResponse resp;
        resp.set_body("hello world");
        EXPECT_EQ(resp.get_body(), "hello world");
    }

    TEST(HttpResponseTest, ContentTypeAndLength) {
        HttpResponse resp;
        resp.set_content_type("application/json");
        resp.set_content_length(123);
        EXPECT_EQ(resp.get_header("Content-Type"), "application/json");
        EXPECT_EQ(resp.get_header("Content-Length"), "123");
    }

    TEST(HttpResponseTest, KeepAlive) {
        HttpResponse resp;
        resp.set_keep_alive(true);
        EXPECT_TRUE(resp.is_keep_alive());
        EXPECT_EQ(resp.get_header("Connection"), "keep-alive");
        resp.set_keep_alive(false);
        EXPECT_FALSE(resp.is_keep_alive());
        EXPECT_EQ(resp.get_header("Connection"), "close");
    }

    TEST(HttpResponseTest, AppendBuffer) {
        HttpResponse resp;
        resp.set_response_line("HTTP/1.1", HttpResponse::StatusCode::OK, "OK");
        resp.set_header("Content-Type", "text/html");
        resp.set_body("abc");
        muduo::net::Buffer buf;
        resp.append_buffer(&buf);
        std::string result(buf.peek(), buf.readableBytes());
        EXPECT_NE(result.find("HTTP/1.1 200 OK"), std::string::npos);
        EXPECT_NE(result.find("Content-Type: text/html"), std::string::npos);
        EXPECT_NE(result.find("\r\n\r\nabc"), std::string::npos);
    }

} // namespace zhttp