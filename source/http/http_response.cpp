#include "../../include/http/http_response.h"

namespace zhttp
{
    // 设置与获取http版本
    void HttpResponse::set_version(const std::string_view &version)
    {
        version_ = std::string(version.begin(), version.end());
    }

    const std::string &HttpResponse::get_version() const
    {
        return version_;
    }

    // 设置与获取响应状态码
    void HttpResponse::set_status_code(HttpResponse::StatusCode status_code)
    {
        status_code_ = status_code;
    }

    HttpResponse::StatusCode HttpResponse::get_status_code() const
    {
        return status_code_;
    }

    // 设置与获取响应状态消息
    void HttpResponse::set_status_message(const std::string_view &status_message)
    {
        status_message_ = std::string(status_message.begin(), status_message.end());
    }

    const std::string &HttpResponse::get_status_message() const
    {
        return status_message_;
    }

    void HttpResponse::set_response_line(const std::string_view &version,
                                         HttpResponse::StatusCode status_code,
                                         const std::string_view &status_message)

    {
        set_version(version);
        set_status_code(status_code);
        set_status_message(status_message);
    }


// 设置与获取响应头
    void HttpResponse::set_header(const std::string_view &key, const std::string_view &value)
    {
        headers_[std::string(key.begin(), key.end())] =
                std::string(value.begin(), value.end());
    }

    std::string HttpResponse::get_header(const std::string &key) const
    {
        auto it = headers_.find(key);
        if (it != headers_.end())
        {
            return it->second;
        }
        return "";
    }

// 设置与获取响应正文
    void HttpResponse::set_body(const std::string_view &body)
    {
        body_ = std::string(body.begin(), body.end());
    }

    const std::string &HttpResponse::get_body() const
    {
        return body_;
    }

// 设置相应正文类型
    void HttpResponse::set_content_type(const std::string_view &content_type)
    {
        set_header("Content-Type", content_type);
    }

// 设置相应正文长度
    void HttpResponse::set_content_length(uint64_t length)
    {
        set_header("Content-Length", std::to_string(length));
    }

// 设置与获取是否保持连接
    void HttpResponse::set_keep_alive(bool is_keep_alive)
    {
        is_keep_alive_ = is_keep_alive;
        if (is_keep_alive_)
        {
            set_header("Connection", "keep-alive");
        }
        else
        {
            set_header("Connection", "close");
        }
    }

    bool HttpResponse::is_keep_alive() const
    {
        return is_keep_alive_;
    }

    void HttpResponse::append_buffer(muduo::net::Buffer *output) const
    {
        // 响应行
        output->append(version_ + " " + std::to_string(static_cast<int>(status_code_)) + " " +
                       status_message_ + delim);
        // 响应头
        for (const auto &header : headers_)
        {
            output->append(header.first + ": " + header.second + delim);
        }
        // 响应正文
        output->append(delim + get_body());
    }

}// namespace zhttp
