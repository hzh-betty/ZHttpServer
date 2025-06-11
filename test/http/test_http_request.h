#pragma once
#include <gtest/gtest.h>
#include "../../include/http/http_request.h"
#include <muduo/base/Timestamp.h>


namespace zhttp
{
    
    TEST(HttpRequestTest, Method)
    {
        zhttp::HttpRequest req;
        req.set_method(zhttp::HttpRequest::Method::POST);
        EXPECT_EQ(req.get_method(), zhttp::HttpRequest::Method::POST);
    }

    TEST(HttpRequestTest, Path)
    {
        zhttp::HttpRequest req;
        req.set_path("/api/test");
        EXPECT_EQ(req.get_path(), "/api/test");
    }

    TEST(HttpRequestTest, Version)
    {
        zhttp::HttpRequest req;
        req.set_version("HTTP/1.1");
        EXPECT_EQ(req.get_version(), "HTTP/1.1");
    }

    TEST(HttpRequestTest, PathParameters)
    {
        zhttp::HttpRequest req;
        req.set_path_parameters("id", "123");
        EXPECT_EQ(req.get_path_parameters("id"), "123");
        EXPECT_EQ(req.get_path_parameters("not_exist"), "");
    }

    TEST(HttpRequestTest, QueryParameters)
    {
        zhttp::HttpRequest req;
        req.set_query_parameters("page=2&size=10");
        EXPECT_EQ(req.get_query_parameters("page"), "2");
        EXPECT_EQ(req.get_query_parameters("size"), "10");
        EXPECT_EQ(req.get_query_parameters("not_exist"), "");
    }

    TEST(HttpRequestTest, Header)
    {
        zhttp::HttpRequest req;
        req.set_header("Content-Type", "application/json");
        EXPECT_EQ(req.get_header("Content-Type"), "application/json");
        EXPECT_EQ(req.get_header("Not-Exist"), "");
    }

    TEST(HttpRequestTest, Content)
    {
        zhttp::HttpRequest req;
        req.set_content("hello world");
        EXPECT_EQ(req.get_content(), "hello world");
    }

    TEST(HttpRequestTest, ContentLength)
    {
        zhttp::HttpRequest req;
        req.set_content_length(1234);
        EXPECT_EQ(req.get_content_length(), 1234);
    }

    TEST(HttpRequestTest, ReceiveTime)
    {
        zhttp::HttpRequest req;
        muduo::Timestamp t = muduo::Timestamp::now();
        req.set_receive_time(t);
        EXPECT_EQ(req.get_receive_time(), t);
    }
} // namespace zhttp
