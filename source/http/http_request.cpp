#include <algorithm>
#include"../../include/http/http_request.h"

namespace zhttp
{
    // 设置与获取请求方法
    void HttpRequest::set_method(Method method)
    {
        method_ = method;
    }


    HttpRequest::Method HttpRequest::get_method() const
    {
        return method_;
    }


// 设置与获取请求路径
    void HttpRequest::set_path(const std::string_view &path)
    {
        path_ = std::string(path.begin(), path.end());
    }


    const std::string &HttpRequest::get_path() const
    {
        return path_;
    }


    // 设置与获取请求http版本
    void HttpRequest::set_version(const std::string_view &version)
    {
        version_ = std::string(version.begin(), version.end());
    }


    const std::string &HttpRequest::get_version() const
    {
        return version_;
    }


    // 设置与获取请求路径参数
    void HttpRequest::set_path_parameters(const std::string_view &key, const std::string_view &value)
    {
        path_parameters_[std::string(key.begin(), key.end())] = std::string(value.begin(), value.end());
    }


    std::string HttpRequest::get_path_parameters(const std::string &key) const
    {
        if (const auto it = path_parameters_.find(key); it != path_parameters_.end())
        {
            return it->second;
        }
        return "";
    }


    // 设置与获取请求查询参数
    void HttpRequest::set_query_parameters(const std::string_view &str)
    {
        // 解析查询参数
        // 例如：GET /api?page=2&size=10 HTTP/1.1

        size_t pos = 0;
        while (pos < str.size())
        {
            const size_t equal_pos = str.find('=', pos);
            if (equal_pos == std::string_view::npos)
                break;

            size_t amp_pos = str.find('&', equal_pos);
            if (amp_pos == std::string_view::npos)
                amp_pos = str.size();

            std::string key(str.substr(pos, equal_pos - pos));
            const std::string value(str.substr(equal_pos + 1, amp_pos - equal_pos - 1));
            query_parameters_[key] = value;

            pos = amp_pos + 1;
        }

    }


    std::string HttpRequest::get_query_parameters(const std::string &key) const
    {
        if (const auto it = query_parameters_.find(key); it != query_parameters_.end())
        {
            return it->second;
        }
        return "";
    }

    // 设置与获取接收时间
    void HttpRequest::set_receive_time(const muduo::Timestamp &time)
    {
        receive_time_ = time;
    }

    const muduo::Timestamp &HttpRequest::get_receive_time() const
    {
        return receive_time_;
    }

    // 设置与获取请求头
    void HttpRequest::set_header(const std::string_view &key, const std::string_view &value)
    {
        // Content-Length : 1234
        // Content-Type : application/json
        // 排除前后空格
        size_t pos1 = key.size() -1 ;
        while(isspace(key[pos1]) && pos1 > 0)
        {
            --pos1;
        }
        const std::string_view key_view = key.substr(0, pos1 + 1);

        size_t pos2 = 0;
        while(isspace(value[pos2]) && pos2 < value.size())
        {
            ++pos2;
        }
        std::string_view value_view = value.substr(pos2, value.size() - pos2);
        headers_[std::string(key_view.begin(), key_view.end())] =
                std::string(value_view.begin(), value_view.end());
    }

    std::string HttpRequest::get_header(const std::string &key) const
    {
        if (const auto it = headers_.find(key); it != headers_.end())
        {
            return it->second;
        }
        return "";
    }

    // 设置与获取请求体
    void HttpRequest::set_content(const std::string_view &content)
    {
        content_ = std::string(content.begin(), content.end());
    }


    const std::string &HttpRequest::get_content() const
    {
        return content_;
    }


    // 设置与获取请求体长度
    void HttpRequest::set_content_length(uint64_t length)
    {
        content_length_ = length;
    }


    uint64_t HttpRequest::get_content_length() const
    {
        return content_length_;
    }

    void HttpRequest::swap(HttpRequest &other) noexcept
    {
        std::swap(method_, other.method_);
        std::swap(path_, other.path_);
        std::swap(version_, other.version_);
        std::swap(path_parameters_, other.path_parameters_);
        std::swap(query_parameters_, other.query_parameters_);
        std::swap(receive_time_, other.receive_time_);
        std::swap(headers_, other.headers_);
        std::swap(content_, other.content_);
        std::swap(content_length_, other.content_length_);
    }

}// namespace zhttp
