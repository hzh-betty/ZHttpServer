#include "http/http_request.h"
#include "log/http_logger.h"
#include <algorithm>
#include <cctype>

namespace zhttp
{
    // 设置与获取请求方法
    void HttpRequest::set_method(Method method)
    {
        method_ = method;
        ZHTTP_LOG_DEBUG("HTTP request method set to: {}", get_method_string(method_));
    }

    HttpRequest::Method HttpRequest::get_method() const
    {
        return method_;
    }

    std::string HttpRequest::get_method_string(const HttpRequest::Method method)
    {
        switch (method)
        {
        case Method::GET:
            return "GET";
        case Method::POST:
            return "POST";
        case Method::PUT:
            return "PUT";
        case Method::PATCH:
            return "PATCH";
        case Method::HEAD:
            return "HEAD";
        case Method::DELETE:
            return "DELETE";
        case Method::OPTIONS:
            return "OPTIONS";
        default:
            return "INVALID";
        }
    }

    // 设置与获取请求路径
    void HttpRequest::set_path(const std::string_view &path)
    {
        std::string path_tmp = std::string(path.begin(), path.end());
        path_ = url_decode(path_tmp,false);
        ZHTTP_LOG_DEBUG("HTTP request path set to: '{}'", path_);
    }

    const std::string &HttpRequest::get_path() const
    {
        return path_;
    }

    // 设置与获取请求http版本
    void HttpRequest::set_version(const std::string_view &version)
    {
        version_ = std::string(version.begin(), version.end());
        ZHTTP_LOG_DEBUG("HTTP request version set to: '{}'", version_);
    }

    const std::string &HttpRequest::get_version() const
    {
        return version_;
    }

    // 设置与获取请求路径参数
    void HttpRequest::set_path_parameters(const std::string_view &key, const std::string_view &value)
    {
        std::string key_str(key.begin(), key.end());
        std::string value_str(value.begin(), value.end());
        path_parameters_[key_str] = value_str;
        ZHTTP_LOG_DEBUG("HTTP request path parameter set: '{}' = '{}'", key_str, value_str);
    }

    std::string HttpRequest::get_path_parameters(const std::string &key) const
    {
        if (const auto it = path_parameters_.find(key); it != path_parameters_.end())
        {
            ZHTTP_LOG_DEBUG("HTTP request path parameter found: '{}' = '{}'", key, it->second);
            return it->second;
        }
        ZHTTP_LOG_DEBUG("HTTP request path parameter not found: '{}'", key);
        return "";
    }

    // 设置与获取请求查询参数
    void HttpRequest::set_query_parameters(const std::string_view &str)
    {
        ZHTTP_LOG_DEBUG("Parsing query parameters: '{}'", std::string(str));

        query_parameters_.clear();

        // 解析查询参数字符串，格式为：key1=value1&key2=value2
        size_t start = 0;
        size_t param_count = 0;

        while (start < str.length())
        {
            // 查找下一个&或字符串结尾
            size_t end = str.find('&', start);
            if (end == std::string_view::npos)
            {
                end = str.length();
            }

            // 解析单个参数
            std::string_view param = str.substr(start, end - start);
            size_t equals_pos = param.find('=');

            if (equals_pos != std::string_view::npos)
            {
                std::string key(param.substr(0, equals_pos));
                std::string value(param.substr(equals_pos + 1));
                
                // URL解码键和值
                query_parameters_[url_decode(key, true)] = url_decode(value, true);
                param_count++;

                ZHTTP_LOG_DEBUG("Query parameter parsed: '{}' = '{}'", key, value);
            }
            else if (!param.empty())
            {
                // 没有值的参数，设置为空字符串
                std::string key(param);
                query_parameters_[url_decode(key, true)] = "";
                param_count++;

                ZHTTP_LOG_DEBUG("Query parameter parsed (no value): '{}'", key);
            }

            start = end + 1;
        }

        ZHTTP_LOG_INFO("Query parameters parsed successfully, total count: {}", param_count);
    }

    std::string HttpRequest::get_query_parameters(const std::string &key) const
    {
        if (const auto it = query_parameters_.find(key); it != query_parameters_.end())
        {
            ZHTTP_LOG_DEBUG("HTTP request query parameter found: '{}' = '{}'", key, it->second);
            return it->second;
        }
        ZHTTP_LOG_DEBUG("HTTP request query parameter not found: '{}'", key);
        return "";
    }

    // 设置与获取接收时间
    void HttpRequest::set_receive_time(const muduo::Timestamp &time)
    {
        receive_time_ = time;
        ZHTTP_LOG_DEBUG("HTTP request receive time set to: {}", time.toFormattedString());
    }

    const muduo::Timestamp &HttpRequest::get_receive_time() const
    {
        return receive_time_;
    }

    // 设置与获取请求头
    void HttpRequest::set_header(const std::string_view &key, const std::string_view &value)
    {
        // 去除键值的前后空白字符
        std::string trimmed_key(key.begin(), key.end());
        std::string trimmed_value(value.begin(), value.end());

        // 去除前后空白
        auto trim = [](std::string &s)
        {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
                                            { return !std::isspace(ch); }));
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
                                 { return !std::isspace(ch); })
                        .base(),
                    s.end());
        };

        trim(trimmed_key);
        trim(trimmed_value);

        headers_[trimmed_key] = trimmed_value;
        ZHTTP_LOG_DEBUG("HTTP request header set: '{}' = '{}'", trimmed_key, trimmed_value);
    }

    std::string HttpRequest::get_header(const std::string &key) const
    {
        if (const auto it = headers_.find(key); it != headers_.end())
        {
            ZHTTP_LOG_DEBUG("HTTP request header found: '{}' = '{}'", key, it->second);
            return it->second;
        }
        ZHTTP_LOG_DEBUG("HTTP request header not found: '{}'", key);
        return "";
    }

    // 设置与获取请求体
    void HttpRequest::set_content(const std::string_view &content)
    {
        content_ = std::string(content.begin(), content.end());
        ZHTTP_LOG_DEBUG("HTTP request content set, length: {} bytes", content_.size());

        if (!content_.empty())
        {
            // 只显示前100个字符的预览
            std::string preview = content_.substr(0, std::min(size_t(100), content_.size()));
            ZHTTP_LOG_DEBUG("HTTP request content preview: '{}'", preview);
        }
        set_content_length(content_.size());
    }

    const std::string &HttpRequest::get_content() const
    {
        return content_;
    }

    // 设置与获取请求体长度
    void HttpRequest::set_content_length(uint64_t length)
    {
        content_length_ = length;
        ZHTTP_LOG_DEBUG("HTTP request content length set to: {}", length);
    }

    uint64_t HttpRequest::get_content_length() const
    {
        return content_length_;
    }

    void HttpRequest::swap(HttpRequest &other) noexcept
    {
        ZHTTP_LOG_DEBUG("Swapping HTTP request objects");

        std::swap(method_, other.method_);
        path_.swap(other.path_);
        version_.swap(other.version_);
        path_parameters_.swap(other.path_parameters_);
        query_parameters_.swap(other.query_parameters_);
        std::swap(receive_time_, other.receive_time_);
        headers_.swap(other.headers_);
        content_.swap(other.content_);
        std::swap(content_length_, other.content_length_);

        ZHTTP_LOG_DEBUG("HTTP request objects swapped successfully");
    }

    //url_decode 函数把 + 解码为 空格，但实际上 HTTP 路径部分的空格应由 %20 表示，
    //+ 只在 application/x-www-form-urlencoded（表单/查询参数）中代表空格，
    //路径部分不能直接解码 + 为 空格
    std::string HttpRequest::url_decode(const std::string &src,bool plus_to_space)
    {
        ZHTTP_LOG_DEBUG("URL decoding: '{}'", src);
        std::ostringstream oss;
        for (size_t i = 0; i < src.length(); ++i)
        {
            if (src[i] == '%' && i + 2 < src.length())
            {
                int value = 0;
                std::istringstream is(src.substr(i + 1, 2));
                if (is >> std::hex >> value)
                {
                    oss << static_cast<char>(value);
                    i += 2;
                }
                else
                {
                    oss << '%';
                }
            }
            else if (src[i] == '+' &&plus_to_space)
            {
                oss << ' ';
            }
            else
            {
                oss << src[i];
            }
        }
        ZHTTP_LOG_DEBUG("URL decoded: '{}'", oss.str());
        return oss.str();
    }
} // namespace zhttp

