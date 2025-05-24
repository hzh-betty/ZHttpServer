#pragma once

#include <unordered_map>
#include <functional>
#include <string>
#include <regex>
#include <utility>
#include <vector>
#include <memory>
#include<regex>
#include "router_handler.h"


namespace zhttp
{
    /*选择注册对象式的路由处理器还是注册回调函数式的处理器取决于处理器执行的复杂程度
    如果是简单的处理可以注册回调函数，否则注册对象式路由处理器(对象中可封装多个相关函数)*/
    namespace zrouter
    {
        class Router
        {
        public:
            using HandlerPtr = std::shared_ptr<RouterHandler>;
            using HandlerCallback = std::function<void(const HttpRequest &, HttpResponse *)>;
        public:
            // 路由键 
            struct RouteKey
            {
                bool operator==(const RouteKey &other) const
                {
                    return method == other.method && path == other.path;
                }

                HttpRequest::Method method;
                std::string path;
            };

            // 路由键的哈希函数
            struct RouteKeyHash
            {
                std::size_t operator()(const RouteKey &key) const
                {
                    size_t path_hash = std::hash<std::string>()(key.path);
                    size_t method_hash = std::hash<int>()(static_cast<int>(key.method));
                    return path_hash * 31 + method_hash;
                }
            };

            struct RouteCallbackObj
            {
                RouteCallbackObj(std::regex regex_path, const HttpRequest::Method &method, HandlerCallback callback)
                        : regex_path_(std::move(regex_path)), method_(method), callback_(std::move(callback))
                {

                }

                std::regex regex_path_;
                HttpRequest::Method method_;
                HandlerCallback callback_;
            };

            struct RouteHandlerObj
            {
                RouteHandlerObj(std::regex &&regex_path, const HttpRequest::Method &method, HandlerPtr &&handler)
                        : regex_path_(std::move(regex_path)), method_(method), handler_(std::move(handler))
                {

                }

                std::regex regex_path_;
                HttpRequest::Method method_;
                HandlerPtr handler_;
            };

        public:
            // 注册路由处理器
            void register_handler(const std::string &path, const HttpRequest::Method &method, HandlerPtr handler);

            // 注册路由回调函数
            void
            register_callback(const std::string &path, const HttpRequest::Method &method, HandlerCallback callback);

            // 注册动态路由处理器
            void register_regex_handler(const std::string &path, const HttpRequest::Method &method, HandlerPtr handler);

            // 注册动态路由回调函数
            void register_regex_callback(const std::string &path, const HttpRequest::Method &method,
                                         HandlerCallback callback);

            // 路由处理
            bool route(const HttpRequest &request, HttpResponse *response);

        private:
            // 将路径转换为正则表达式
            std::regex convert_to_regex(const std::string &path);

            // 提前路径参数
            void extract_path_parameters(const std::smatch &match, HttpRequest &request);

        private:
            std::unordered_map<RouteKey, HandlerPtr, RouteKeyHash> handlers_;//精确匹配
            std::unordered_map<RouteKey, HandlerCallback, RouteKeyHash> callbacks_;//精确匹配
            std::vector<RouteHandlerObj> regex_handlers_;//正则表达式匹配
            std::vector<RouteCallbackObj> regex_callbacks_;//正则表达式匹配
        };
    }// namespace router

} // namespace zhttp