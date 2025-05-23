#pragma once

#include <muduo/net/TcpServer.h>
#include <string>
#include <unordered_map>

namespace zhttp
{
    /* http响应类 */

    class HttpResponse
    {
    public:
        enum class StatusCode
        {
            UnKnown = 0,
            OK = 200,
            Created = 201,
            Accepted = 202,
            NoContent = 204,
            MovedPermanently = 301,
            Found = 302,
            NotModified = 304,
            BadRequest = 400,
            Unauthorized = 401,
            Forbidden = 403,
            NotFound = 404,
            InternalServerError = 500,
            NotImplemented = 501,
            BadGateway = 502,
        };
    public:
        HttpResponse() = default;

        ~HttpResponse() = default;

    public:
        // 设置与获取http版本
        void set_version(const std::string_view &version);

        const std::string &get_version() const;

        // 设置与获取响应状态码
        void set_status_code(StatusCode status_code);

        StatusCode get_status_code() const;

        // 设置与获取响应状态消息
        void set_status_message(const std::string_view &status_message);

        const std::string &get_status_message() const;

        // 设置响应行
        void set_response_line(const std::string_view &version,
                               StatusCode status_code,
                               const std::string_view &status_message);

        // 设置与获取响应头
        void set_header(const std::string_view &key, const std::string_view &value);

        std::string get_header(const std::string &key) const;

        // 设置与获取响应正文
        void set_body(const std::string_view &body);

        const std::string &get_body() const;

        // 设置相应正文类型
        void set_content_type(const std::string_view &content_type);

        // 设置相应正文长度
        void set_content_length(uint64_t length);

        // 设置与获取是否保持连接
        void set_keep_alive(bool is_keep_alive);

        bool is_keep_alive() const;

        void append_buffer(muduo::net::Buffer *output) const;

    private:
        std::string version_;// http版本
        StatusCode status_code_ = StatusCode::UnKnown;// 响应状态码
        std::string status_message_;// 响应状态消息
        std::unordered_map<std::string, std::string> headers_;// 响应头
        std::string body_;// 响应正文
        bool is_keep_alive_ = false;// 是否保持连接
    };

    // 每行之间的分隔符
    inline const std::string delim = "\r\n";
} // namespace zhttp
