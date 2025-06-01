#pragma once

#include <map>
#include <unordered_map>
#include <string>
#include <muduo/base/Timestamp.h>

namespace zhttp
{
    /* HttpRequest负责解析Http请求 */
    class HttpRequest
    {
    public:
        enum class Method
        {
            Invalid,
            GET,
            POST,
            PATCH,
            HEAD,
            PUT,
            DELETE,
            OPTIONS,
        };

        HttpRequest() = default;
        ~HttpRequest() = default;
    public:
        // 设置与获取请求方法
        void set_method(Method method);
        Method get_method() const ;

        // 设置与获取请求路径
        void set_path(const std::string_view &path);
        const std::string &get_path() const ;

        // 设置与获取请求http版本
        void set_version(const std::string_view &version);
        const std::string &get_version() const ;

        // 设置与获取请求路径参数
        void set_path_parameters(const std::string_view&key, const std::string_view& value);
        std::string get_path_parameters(const std::string &key) const;

        // 设置与获取请求查询参数
        void set_query_parameters(const std::string_view &str);
        std::string get_query_parameters(const std::string &key) const;

        // 设置与获取接收时间
        void set_receive_time(const muduo::Timestamp &time);
        const muduo::Timestamp &get_receive_time() const;

        // 设置与获取请求头
        void set_header(const std::string_view &key, const std::string_view &value);
        std::string get_header(const std::string &key) const;

        // 设置与获取请求体
        void set_content(const std::string_view &content);
        const std::string &get_content() const;

        // 设置与获取请求体长度
        void set_content_length(uint64_t length);
        uint64_t get_content_length() const;

        void swap(HttpRequest&other);
    private:
        Method method_ = Method::Invalid;// 请求方法
        std::string path_;// 请求路径
        std::string version_;// 协议版本
        std::unordered_map<std::string, std::string> path_parameters_;// 路径参数
        std::unordered_map<std::string, std::string> query_parameters_; // 查询参数
        muduo::Timestamp receive_time_; // 接收时间
        std::map<std::string, std::string> headers_; // 请求头
        std::string content_; // 请求体
        uint64_t content_length_ = 0; // 请求体长度
    };
}// namespace zhttp