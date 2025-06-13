#include "http/http_context.h"
#include "log/http_logger.h"

namespace zhttp
{
    bool HttpContext::parse_request(muduo::net::Buffer *buffer, muduo::Timestamp receive_time)
    {

        ZHTTP_LOG_DEBUG("Starting HTTP request parsing, buffer size: {} bytes", buffer->readableBytes());

        bool check = true; // 解析数据是否正确
        bool loop = true;  // 是否需要继续解析
        
        while (loop)
        {
            // 如果解析状态是ExpectComplete，表示已经完成了请求行和头部的解析
            if (state_ == HttpRequestParseState::ExpectComplete)
            {
                ZHTTP_LOG_DEBUG("Request already parsed, checking for body");
                break;
            }

            // 如果已经到了解析请求体阶段
            if (state_ == HttpRequestParseState::ExpectBody)
            {
                ZHTTP_LOG_DEBUG("Parsing request body, expected length: {}", request_.get_content_length());
                parse_body(buffer);
                break; // 解析请求体后退出循环
            }
            
            // 查找\r\n
            if (const char *crlf = buffer->findCRLF())
            {
                std::string_view line(buffer->peek(), crlf - buffer->peek());// 获取一行数据
                buffer->retrieveUntil(crlf + 2);//标记为已读
                
                ZHTTP_LOG_DEBUG("Parsing line: '{}'", std::string(line));
                
                switch (state_)
                {
                    case HttpRequestParseState::ExpectRequestLine:
                        check = loop = parse_request_line(line, receive_time);
                        if (!check) {
                            ZHTTP_LOG_ERROR("Failed to parse request line: '{}'", std::string(line));
                        }
                        break;
                    case HttpRequestParseState::ExpectHeaders:
                        ZHTTP_LOG_DEBUG("Parsing header line : {}", std::string(line));
                        check = loop = parse_headers(line);
                        if (!check) {
                            ZHTTP_LOG_ERROR("Failed to parse header line: '{}'", std::string(line));
                        }
                        break;
                    default:
                        ZHTTP_LOG_WARN("Unexpected parse state: {}", static_cast<int>(state_));
                        loop = false;
                        break;
                }
            }
            else
            {
                // 没有找到\r\n，继续读取数据
                ZHTTP_LOG_DEBUG("No CRLF found, waiting for more data. Current buffer size: {}", buffer->readableBytes());
                check = false;
                break;
            }
        }

        ZHTTP_LOG_INFO("HTTP request parsing completed, success: {}, parse complete: {}",
                       check, is_parse_complete());
        return check;
    }

    bool HttpContext::parse_request_line(const std::string_view &line, const muduo::Timestamp &receive_time)
    {
        ZHTTP_LOG_DEBUG("Parsing request line: '{}'", std::string(line));
        
        // 解析请求行
        // GET /index.html HTTP/1.1
        // 解析方法、路径、协议版本

        // 1. 查找空格并设置请求方法
        const size_t pos = line.find(' ');

        if (pos == std::string_view::npos)
        {
            ZHTTP_LOG_ERROR("No space found in request line, invalid format");
            return false;
        }

        const std::string_view method = line.substr(0, pos);
        ZHTTP_LOG_DEBUG("Parsed method: '{}'", std::string(method));
        
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
        {
            ZHTTP_LOG_ERROR("Unsupported HTTP method: '{}'", std::string(method));
            return false;
        }

        // 2. 解析请求路径，包括请求带有参数
        // 例如：GET /api?page=2&size=10 HTTP/1.1

        const size_t pos1 = line.find('?');
        const size_t pos2 = line.find(' ', pos + 1);//第二个空格
        if (pos2 == std::string_view::npos)
        {
            ZHTTP_LOG_ERROR("No second space found in request line, invalid format");
            return false;
        }

        if (pos1 != std::string_view::npos)
        {
            // 解析路径参数并设置路径
            std::string_view path = line.substr(pos + 1, pos1 - pos - 1);
            request_.set_path(path);
            ZHTTP_LOG_DEBUG("Parsed path with query: '{}'", std::string(path));

            // 设置路径参数
            std::string_view path_parameters = line.substr(pos1 + 1, pos2 - pos1 - 1);
            request_.set_query_parameters(path_parameters);
            ZHTTP_LOG_DEBUG("Parsed query parameters: '{}'", std::string(path_parameters));
        }
        else
        {
            std::string_view path = line.substr(pos + 1, pos2 - pos - 1);
            request_.set_path(path);
            ZHTTP_LOG_DEBUG("Parsed path: '{}'", std::string(path));
        }

        // 3. 解析协议版本
        const std::string_view version = line.substr(pos2 + 1);
        ZHTTP_LOG_DEBUG("Parsed version: '{}'", std::string(version));
        
        if (version == "HTTP/1.0")
            request_.set_version("HTTP/1.0");
        else if (version == "HTTP/1.1")
            request_.set_version("HTTP/1.1");
        else
        {
            ZHTTP_LOG_ERROR("Unsupported HTTP version: '{}'", std::string(version));
            return false;
        }

        state_ = HttpRequestParseState::ExpectHeaders;
        request_.set_receive_time(receive_time);
        
        ZHTTP_LOG_INFO("Request line parsed successfully: {} {} {}", 
                      std::string(method), request_.get_path(), std::string(version));
        return true;
    }

    bool HttpContext::parse_headers(const std::string_view &line)
    {
        ZHTTP_LOG_DEBUG("Parsing header line: '{}'", std::string(line));
        
        // 解析请求头
        // Content-Length: 1234
        auto colon = line.find(':', 0);

        // 查找到一个报头
        if (colon != std::string_view::npos)
        {
            const std::string_view first = line.substr(0, colon);
            const std::string_view second = line.substr(colon + 1);
            request_.set_header(first, second);
            
            ZHTTP_LOG_DEBUG("Header parsed: '{}' = '{}'", std::string(first), std::string(second));
            return true;
        }
        // 解析到空行，表示请求头结束
        else if (line.empty())
        {
            ZHTTP_LOG_DEBUG("Empty line encountered, headers parsing complete");
            
            // 检查是否有Content-Length头部
            const std::string content_length_str = request_.get_header("Content-Length");
            if (!content_length_str.empty())
            {
                try {
                    uint64_t content_length = std::stoull(content_length_str);
                    request_.set_content_length(content_length);
                    ZHTTP_LOG_DEBUG("Content-Length set to: {}", content_length);
                } catch (const std::exception& e) {
                    // Content-Length 格式错误
                    ZHTTP_LOG_ERROR("Invalid Content-Length format: '{}', error: {}", 
                                   content_length_str, e.what());
                    return false;
                }
            }

            // 如果没有请求体，直接完成解析
            if (request_.get_content_length() == 0)
            {
                ZHTTP_LOG_DEBUG("No request body expected, parsing complete");
                state_ = HttpRequestParseState::ExpectComplete;
            }
            else
            {
                ZHTTP_LOG_DEBUG("Request body expected, switching to body parsing state");
                state_ = HttpRequestParseState::ExpectBody;
            }
            return true;
        }

        // 报头不完整或者格式错误
        ZHTTP_LOG_ERROR("Invalid header format, no colon found: '{}'", std::string(line));
        return false;
    }

    // 解析请求体
    void HttpContext::parse_body(muduo::net::Buffer *buffer)
    {
        ZHTTP_LOG_DEBUG("Parsing request body, expected length: {}, available: {}", 
                       request_.get_content_length(), buffer->readableBytes());
        
        // 如果没有Content-Length，直接完成解析
        if (request_.get_content_length() == 0)
        {
            ZHTTP_LOG_DEBUG("No content length specified, completing parse");
            state_ = HttpRequestParseState::ExpectComplete;
            return;
        }

        // 如果请求体长度为0，或者buffer数据足够
        if (buffer->readableBytes() >= request_.get_content_length())
        {
            const std::string_view content(buffer->peek(), request_.get_content_length());
            request_.set_content(content);
            buffer->retrieve(request_.get_content_length());
            state_ = HttpRequestParseState::ExpectComplete;
            
            ZHTTP_LOG_INFO("Request body parsed successfully, length: {} bytes", 
                          request_.get_content_length());
            ZHTTP_LOG_DEBUG("Request body content preview: '{}'", 
                           std::string(content).substr(0, std::min(size_t(100), content.size())));
        }
        else
        {
            // 如果数据不够，继续等待
            ZHTTP_LOG_DEBUG("Insufficient data for request body, waiting for more. Need: {}, have: {}", 
                           request_.get_content_length(), buffer->readableBytes());
        }
    }

    bool HttpContext::is_parse_complete() const
    {
        bool complete = (state_ == HttpRequestParseState::ExpectComplete);
        ZHTTP_LOG_DEBUG("Parse complete check: {}", complete ? "true" : "false");
        return complete;
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
        ZHTTP_LOG_DEBUG("Resetting HTTP context to initial state");
        state_ = HttpRequestParseState::ExpectRequestLine;
        HttpRequest other;
        request_.swap(other);
        ZHTTP_LOG_DEBUG("HTTP context reset completed");
    }

} // namespace zhttp