#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <functional>
#include <memory>
#include <unordered_map>

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Logging.h>

#include "http_context.h"
#include "http_request.h"
#include "http_response.h"
#include "../middleware/middleware_chain.h"
#include "../router/router.h"
#include "../ssl/ssl_context.h"
#include "../ssl/ssl_connection.h"


namespace zhttp
{
    class HttpServer : public muduo::noncopyable
    {
    public:
        using HttpCallback = std::function<void(const zhttp::HttpRequest &, zhttp::HttpResponse *)>;

        HttpServer(uint16_t port,
                   const std::string &name,
                   bool use_ssl = false,
                   muduo::net::TcpServer::Option option = muduo::net::TcpServer::kNoReusePort);

        // 启动线程数
        void set_thread_num(uint32_t num);

        // 启动
        void start();

        // 获取主线程loop
        muduo::net::EventLoop *get_main_loop() ;

        // 注册静态路由回调
        void Get(const std::string &path, const HttpCallback &cb);

        void Get(const std::string &path, zrouter::Router::HandlerPtr handler);

        void Post(const std::string &path, const HttpCallback &cb);

        void Post(const std::string &path, zrouter::Router::HandlerPtr handler);

        // 注册动态路由回调
        void add_regex_route(HttpRequest::Method method, const std::string &path, const HttpCallback &cb);

        void add_regex_route(HttpRequest::Method method, const std::string &path, zrouter::Router::HandlerPtr handler);

        // 添加中间件
        void add_middleware(const std::shared_ptr<zmiddleware::Middleware>& middleware);

        // 启动SSL
        void enable_ssl(bool enable);

        // 添加SSL上下文
        void set_ssl_context();
    private:
        // 初始化
        void init();

        // 新链接建立与断开回调
        void on_connection(const muduo::net::TcpConnectionPtr &conn);

        // 接受到数据回调
        void on_message(const muduo::net::TcpConnectionPtr &conn,
                        muduo::net::Buffer *buf,
                        muduo::Timestamp receive_time);

        // 得到一个完整的HTTP请求后的回调处理
        void on_request(const muduo::net::TcpConnectionPtr &conn,
                        zhttp::HttpRequest &request);

        // 中间件-路由-中间件处理
        void handle_request(const zhttp::HttpRequest &request,
                zhttp::HttpResponse *response);

    private:
        muduo::net::InetAddress listen_addr_; // 监听地址
        muduo::net::TcpServer server_; // tcp server
        muduo::net::EventLoop main_loop_; // 主线程loop
        HttpCallback callback_; // 默认回调函数
        zrouter::Router router_; // 路由
        zmiddleware::MiddlewareChain middleware_chain_; // 中间件链
        std::unique_ptr<zssl::SslContext> ssl_context_; // SSL上下文
        bool is_ssl_ = false; // 是否启用SSL
        std::unordered_map<muduo::net::TcpConnectionPtr, std::unique_ptr<zssl::SslConnection>> ssl_connections_;
    };
} // namespace zhttp
