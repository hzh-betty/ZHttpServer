#include "../../include/router/router.h"


namespace zhttp::zrouter
{
    // 注册路由处理器
    void Router::register_handler(const std::string &path, const HttpRequest::Method &method,
                                  Router::HandlerPtr handler)
    {
        Router::RouteKey key{method, path};
        handlers_[key] = std::move(handler);
    }


    // 注册路由回调函数
    void Router::register_callback(const std::string &path, const HttpRequest::Method &method,
                                   Router::HandlerCallback callback)
    {
        Router::RouteKey key{method, path};
        callbacks_[key] = std::move(callback);
    }

    // 注册动态路由处理器
    void Router::register_regex_handler(const std::string &path, const HttpRequest::Method &method,
                                        Router::HandlerPtr handler)
    {
        regex_handlers_.emplace_back(convert_to_regex(path), method, std::move(handler));
    }


    // 注册动态路由回调函数
    void Router::register_regex_callback(const std::string &path, const HttpRequest::Method &method,
                                         Router::HandlerCallback callback)
    {
        regex_callbacks_.emplace_back(convert_to_regex(path), method, std::move(callback));
    }


    // 路由处理
    bool Router::route(const HttpRequest &request, HttpResponse *response)
    {
        Router::RouteKey key{request.get_method(), request.get_path()};

        // 1.查找处理器
        auto it = handlers_.find(key);
        if (it != handlers_.end())
        {
            it->second->handle_request(request, response);
            return true;
        }

        // 2.查找回调函数
        auto it_callback = callbacks_.find(key);
        if (it_callback != callbacks_.end())
        {
            it_callback->second(request, response);
            return true;
        }

        // 3.查找正则表达式处理器
        for (const auto &[regex_path, method, handler]: regex_handlers_)
        {
            std::smatch match;
            if (std::regex_match(request.get_path(), match, regex_path) && method == request.get_method())
            {
                // 提取路径参数
                HttpRequest new_request = request;
                extract_path_parameters(match, new_request);
                handler->handle_request(new_request, response);
                return true;
            }
        }

        // 4.查找正则表达式回调函数
        for (const auto &[regex_path, method, callback]: regex_callbacks_)
        {
            std::smatch match;
            if (std::regex_match(request.get_path(), match, regex_path) && method == request.get_method())
            {
                // 提取路径参数
                HttpRequest new_request = request;
                extract_path_parameters(match, new_request);
                callback(new_request, response);
                return true;
            }
        }
        return false;
    }

    // 将路径转换为正则表达式
    //输入路径模式：/users/:id/posts/:postId
    //转换后的正则表达式：^/users/([^/]+)/posts/([^/]+)$
    //可匹配的路径示例：/users/123/posts/456
    std::regex Router::convert_to_regex(const std::string &path)
    {
        std::string regex_pattern = "^" + std::regex_replace(path,
                                                             std::regex(R"(/:([^/]+))"),
                                                             R"(/([^/]+))") + "$";
        return std::regex(regex_pattern);
    }


    // 提前路径参数
    void Router::extract_path_parameters(const std::smatch &match, HttpRequest &request)
    {
        // 跳过索引 0，因为索引 0 存储的是整个匹配的路径字符串，并非捕获组。
        for (size_t i = 1; i < match.size(); ++i)
        {
            request.set_path_parameters("param" + std::to_string(i), match[i].str());
        }
    }

} // namespace zhttp::zrouter


