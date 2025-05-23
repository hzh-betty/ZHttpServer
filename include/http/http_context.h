#pragma once
#include "http_request.h"
#include <muduo/net/TcpServer.h>
#include <string_view>

/* HttpContext负责检查客户端传来的Http请求是否符合规范 */
namespace zhttp
{

    class HttpContext
    {
        enum class HttpRequestParseState
        {
            ExpectRequestLine,// 解析请求行
            ExpectHeaders,// 解析请求头
            ExpectBody,// 解析请求体
            ExpectComplete// 解析完成
        };

    public:
        HttpContext() = default;
        ~HttpContext() = default;
    public:
        // 将报文解析成HttpRequest对象
        bool parse_request(muduo::net::Buffer *buffer,muduo::Timestamp receive_time);

        // 解析是否完成
        bool is_parse_complete() const;
    private:
        // 解析请求行
        bool parse_request_line(const std::string_view &line, const muduo::Timestamp& receive_time);
        // 解析请求头
        bool parse_headers(const std::string_view &line);
        // 解析请求体
        void parse_body(muduo::net::Buffer *buffer);

    private:
        HttpRequestParseState state_ = HttpRequestParseState::ExpectRequestLine; // 当前解析状态
        HttpRequest request_;// 当前请求
    };
}// namespace zhttp
