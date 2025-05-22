#include"../include/http_request.h"

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

    }


// 设置与获取请求http版本
    void HttpRequest::set_version(const std::string_view &version)
    {

    }


    const std::string &HttpRequest::get_version() const {}


// 设置与获取请求路径参数
    void HttpRequest::set_path_parameters(const std::string_view &key, const std::string_view &value) {}


    std::string HttpRequest::get_path_parameters(const std::string &key) const {}


// 设置与获取请求查询参数
    void HttpRequest::set_qurey_parameters(const std::string_view &str) {}


    std::string HttpRequest::get_qurey_parameters(const std::string &key) const {}

// 设置与获取接收时间
    void HttpRequest::set_receive_time(const muduo::Timestamp &time) {}

    const muduo::Timestamp &HttpRequest::get_receive_time() const {}

// 设置与获取请求头
    void HttpRequest::set_header(const std::string_view &key, const std::string_view &value)
    {

    }

    std::string HttpRequest::get_header(const std::string &key) const
    {

    }

// 设置与获取请求体
    void HttpRequest::set_content(const std::string_view &content) {}


    const std::string &HttpRequest::get_content() const {}


// 设置与获取请求体长度
    void HttpRequest::set_content_length(uint64_t length) {}


    uint64_t HttpRequest::get_content_length() const
    {

    }

}// namespace zhttp
