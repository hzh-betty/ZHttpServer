#include "../../include/http/http_response.h"
#include "../../include/log/logger.h"

namespace zhttp
{
    // 设置与获取http版本
    void HttpResponse::set_version(const std::string_view &version)
    {
        version_ = std::string(version.begin(), version.end());
        ZHTTP_LOG_DEBUG("HTTP response version set to: {}", version_);
    }

    const std::string &HttpResponse::get_version() const
    {
        return version_;
    }

    // 设置与获取响应状态码
    void HttpResponse::set_status_code(HttpResponse::StatusCode status_code)
    {
        status_code_ = status_code;
        ZHTTP_LOG_DEBUG("HTTP response status code set to: {}", static_cast<int>(status_code));
    }

    HttpResponse::StatusCode HttpResponse::get_status_code() const
    {
        return status_code_;
    }

    // 设置与获取响应状态消息
    void HttpResponse::set_status_message(const std::string_view &status_message)
    {
        status_message_ = std::string(status_message.begin(), status_message.end());
        ZHTTP_LOG_DEBUG("HTTP response status message set to: {}", status_message_);
    }

    const std::string &HttpResponse::get_status_message() const
    {
        return status_message_;
    }

    void HttpResponse::set_response_line(const std::string_view &version,
                                         HttpResponse::StatusCode status_code,
                                         const std::string_view &status_message)
    {
        ZHTTP_LOG_DEBUG("Setting HTTP response line: {} {} {}", 
                       version, static_cast<int>(status_code), status_message);
        set_version(version);
        set_status_code(status_code);
        set_status_message(status_message);
    }

    // 设置与获取响应头
    void HttpResponse::set_header(const std::string_view &key, const std::string_view &value)
    {
        ZHTTP_LOG_DEBUG("Setting HTTP response header: {} = {}", key, value);
        headers_[std::string(key.begin(), key.end())] =
                std::string(value.begin(), value.end());
    }

    std::string HttpResponse::get_header(const std::string &key) const
    {
        const auto it = headers_.find(key);
        if (it != headers_.end())
        {
            ZHTTP_LOG_DEBUG("HTTP response header found: {} = {}", key, it->second);
            return it->second;
        }
        ZHTTP_LOG_DEBUG("HTTP response header not found: {}", key);
        return "";
    }

    // 设置与获取响应正文
    void HttpResponse::set_body(const std::string_view &body)
    {
        body_ = std::string(body.begin(), body.end());
        set_content_length(body_.size());
        ZHTTP_LOG_DEBUG("HTTP response body set, length: {} bytes", body_.size());
    }

    const std::string &HttpResponse::get_body() const
    {
        return body_;
    }

    // 设置相应正文类型
    void HttpResponse::set_content_type(const std::string_view &content_type)
    {
        ZHTTP_LOG_DEBUG("Setting HTTP response content type: {}", content_type);
        set_header("Content-Type", content_type);
    }

    // 设置相应正文长度
    void HttpResponse::set_content_length(uint64_t length)
    {
        ZHTTP_LOG_DEBUG("Setting HTTP response content length: {}", length);
        set_header("Content-Length", std::to_string(length));
    }

    // 设置与获取是否保持连接
    void HttpResponse::set_keep_alive(bool is_keep_alive)
    {
        is_keep_alive_ = is_keep_alive;
        ZHTTP_LOG_DEBUG("HTTP response keep-alive set to: {}", is_keep_alive ? "true" : "false");
        
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
        ZHTTP_LOG_DEBUG("Appending HTTP response to buffer");
        
        // 响应行
        std::string response_line = version_ + " " + std::to_string(static_cast<int>(status_code_)) + " " + status_message_ + delim;
        output->append(response_line);
        
        ZHTTP_LOG_DEBUG("Response line appended: {}", response_line.substr(0, response_line.length() - 2)); // 去掉\r\n显示
        
        // 响应头
        size_t header_count = 0;
        for (const auto &header: headers_)
        {
            std::string header_line = header.first + ": " + header.second + delim;
            output->append(header_line);
            header_count++;
        }
        
        ZHTTP_LOG_DEBUG("Appended {} response headers", header_count);
        
        // 响应正文
        output->append(delim + get_body());
        
        ZHTTP_LOG_DEBUG("HTTP response buffer completed, total size: {} bytes", output->readableBytes());
    }

    void HttpResponse::set_request_origin(const std::string &origin)
    {
        request_origin_ = origin;
        if (!origin.empty())
        {
            ZHTTP_LOG_DEBUG("HTTP response request origin set to: {}", origin);
        }
    }

    const std::string &HttpResponse::get_request_origin() const
    {
        return  request_origin_;
    }

    std::string HttpResponse::to_http_date(const muduo::Timestamp &time)
    {
        const time_t seconds = time.secondsSinceEpoch();
        struct tm tm_time{};
        gmtime_r(&seconds, &tm_time);  // 使用 gmtime_r 确保线程安全

        char buf[32];
        strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm_time);
        return buf;
    }

}// namespace zhttp
