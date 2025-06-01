#include "../../include/http/http_context.h"

namespace zhttp
{
    bool HttpContext::parse_request(muduo::net::Buffer *buffer, muduo::Timestamp receive_time)
    {
        if (buffer->readableBytes() == 0)
            return false;

        bool check = true; // 解析数据是否正确
        bool loop = true; // 是否需要继续解析
        while (loop)
        {
            // 查找\r\n
            const char *crlf = buffer->findCRLF();
            if (crlf)
            {
                std::string_view line(buffer->peek(), crlf - buffer->peek());// 获取一行数据
                buffer->retrieveUntil(crlf + 2);//标记为已读
                switch (state_)
                {
                    case HttpRequestParseState::ExpectRequestLine:
                        check = loop = parse_request_line(line, receive_time);
                        break;
                    case HttpRequestParseState::ExpectHeaders:
                        check = loop = parse_headers(line);
                        break;
                    default:
                        break;
                }
            }
            else
            {
                if (state_ == HttpRequestParseState::ExpectBody)
                {
                    // 解析请求体
                    parse_body(buffer);
                }
                else
                {
                    // 没有找到\r\n，继续读取数据
                    check = false;
                }
                break;
            }
        }

        return check;
    }

    bool HttpContext::parse_request_line(const std::string_view &line, const muduo::Timestamp &receive_time)
    {
        // 解析请求行
        // GET /index.html HTTP/1.1
        // 解析方法、路径、协议版本

        // 1. 查找空格并设置请求方法
        size_t pos = line.find(' ');

        if (pos == std::string_view::npos)
            return false;
        std::string_view method = line.substr(0, pos);

        if (method == "GET")
            request_.set_method(HttpRequest::Method::GET);
        else if (method == "POST")
            request_.set_method(HttpRequest::Method::POST);
        else if (method == "PUT")
            request_.set_method(HttpRequest::Method::PUT);
        else if (method == "PATCH")
            request_.set_method(HttpRequest::Method::PATCH);
        else if (method == "HEAD")
            request_.set_method(HttpRequest::Method::HEAD);
        else if (method == "DELETE")
            request_.set_method(HttpRequest::Method::DELETE);
        else if (method == "OPTIONS")
            request_.set_method(HttpRequest::Method::OPTIONS);
        else
            return false;

        // 2. 解析请求路径，包括请求带有参数
        // 例如：GET /api?page=2&size=10 HTTP/1.1

        size_t pos1 = line.find('?');
        size_t pos2 = line.find(' ', pos + 1);//第二个空格
        if (pos2 == std::string_view::npos)
            return false;

        if (pos1 != std::string_view::npos)
        {
            // 解析路径参数并设置路径
            std::string_view path = line.substr(pos + 1, pos1 - pos - 1);
            request_.set_path(path);

            // 设置路径参数
            std::string_view path_parameters = line.substr(pos1 + 1, pos2 - pos1 - 1);
            request_.set_query_parameters(path_parameters);
        }
        else
        {
            std::string_view path = line.substr(pos + 1, pos2 - pos - 1);
            request_.set_path(path);
        }

        // 3. 解析协议版本
        std::string_view version = line.substr(pos2 + 1);
        if (version == "HTTP/1.0")
            request_.set_version("HTTP/1.0");
        else if (version == "HTTP/1.1")
            request_.set_version("HTTP/1.1");
        else
            return false;

        state_ = HttpRequestParseState::ExpectHeaders;
        request_.set_receive_time(receive_time);
        return true;
    }

    bool HttpContext::parse_headers(const std::string_view &line)
    {
        // 解析请求头
        // Content-Length: 1234
        auto colon = line.find(':', 0);

        // 查找到一个报头
        if (colon != std::string_view::npos)
        {
            std::string_view first = line.substr(0, colon);
            std::string_view second = line.substr(colon + 1);
            request_.set_header(first, second);
            return true;
        }
            // 解析到空行，表示请求头结束
        else if (line.empty())
        {
            if (!request_.get_header("Content-Length").empty())
            {
                // 解析请求体
                request_.set_content_length(std::stoul(request_.get_header("Content-Length")));
            }

            state_ = HttpRequestParseState::ExpectBody;
            return true;
        }

        // 报头不完整或者格式错误
        return false;
    }

    // 解析请求体
    void HttpContext::parse_body(muduo::net::Buffer *buffer)
    {
        // 解析请求体
        // Content-Length: 1234
        // Content: hello world

        // 如果请求体长度为0，或者buffer数据足够
        if (buffer->readableBytes() >= request_.get_content_length())
        {
            std::string_view content(buffer->peek(), request_.get_content_length());
            request_.set_content(content);
            buffer->retrieve(request_.get_content_length());
            state_ = HttpRequestParseState::ExpectComplete;
        }
    }

    bool HttpContext::is_parse_complete() const
    {
        return state_ == HttpRequestParseState::ExpectComplete;
    }

    const HttpRequest &HttpContext::request() const
    {
        return request_;
    }

    HttpRequest &HttpContext::request()
    {
        return request_;
    }

    void HttpContext::reset()
    {
        state_ = HttpRequestParseState::ExpectRequestLine;
        HttpRequest other;
        request_.swap(other);
    }


} // namespace zhttp