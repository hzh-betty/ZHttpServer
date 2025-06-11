#pragma once
#include <functional>
#include <memory>
#include <unordered_map>
#include <thread>

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include "../log/logger.h"

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
        void set_thread_num(uint32_t num) const;

        // 启动
        void start() const;

        // 注册静态路由回调
        void Get(const std::string &path, const HttpCallback &cb) const;

        void Get(const std::string &path, zrouter::Router::HandlerPtr handler) const;

        void Post(const std::string &path, const HttpCallback &cb) const;

        void Post(const std::string &path, zrouter::Router::HandlerPtr handler) const;

        // PUT 请求注册
        void Put(const std::string &path, const HttpCallback &cb) const;

        void Put(const std::string &path, zrouter::Router::HandlerPtr handler) const;

        // DELETE 请求注册
        void Delete(const std::string &path, const HttpCallback &cb) const;

        void Delete(const std::string &path, zrouter::Router::HandlerPtr handler) const;

        // PATCH 请求注册
        void Patch(const std::string &path, const HttpCallback &cb) const;

        void Patch(const std::string &path, zrouter::Router::HandlerPtr handler) const;

        // HEAD 请求注册
        void Head(const std::string &path, const HttpCallback &cb) const;

        void Head(const std::string &path, zrouter::Router::HandlerPtr handler) const;

        // OPTIONS 请求注册
        void Options(const HttpCallback &cb) const;

        void Options(zrouter::Router::HandlerPtr handler) const;

        // 注册动态路由回调
        void add_regex_route(HttpRequest::Method method, const std::string &path, const HttpCallback &cb) const;

        void add_regex_route(HttpRequest::Method method, const std::string &path,
                             zrouter::Router::HandlerPtr handler) const;

        // 添加中间件
        void add_middleware(std::shared_ptr<zmiddleware::Middleware> middleware) const;

        // 添加SSL上下文
        void set_ssl_context();

    private:
        // 初始化
        void init(uint16_t port, const std::string &name, muduo::net::TcpServer::Option option);

        // 新链接建立与断开回调
        void on_connection(const muduo::net::TcpConnectionPtr &conn);

        // 接受到数据回调
        void on_message(const muduo::net::TcpConnectionPtr &conn,
                        muduo::net::Buffer *buf,
                        muduo::Timestamp receive_time);

        // 得到一个完整的HTTP请求后的回调处理
        void on_request(const muduo::net::TcpConnectionPtr &conn,
                        const zhttp::HttpRequest &request);

        // 中间件-路由-中间件处理
        void handle_request(const zhttp::HttpRequest &request,
                            zhttp::HttpResponse *response) const;

        // 向客户端响应数据
        void send(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer &output);

    private:
        std::unique_ptr<muduo::net::InetAddress> listen_addr_;           // 监听地址
        std::unique_ptr<muduo::net::EventLoop> main_loop_;               // 主线程loop
        std::unique_ptr<muduo::net::TcpServer> server_;                  // tcp server
        std::unique_ptr<zrouter::Router> router_;                        // 路由
        std::unique_ptr<zmiddleware::MiddlewareChain> middleware_chain_; // 中间件链
        std::unique_ptr<zssl::SslContext> ssl_context_;                  // SSL上下文
        std::unordered_map<muduo::net::TcpConnectionPtr, std::unique_ptr<zssl::SslConnection>> ssl_connections_;
        HttpCallback callback_;                                      // 默认回调函数
        bool is_ssl_ = false;                                        // 是否启用SSL
        inline static std::string options_path_ = "/options/method"; // OPTIONS请求的路径
    };

    // 建造者模式
    class ServerBuilder
    {
    public:
        virtual ~ServerBuilder() = default;

        // 建造证书文件路径
        void build_cert_file_path(const std::string &cert_file_path)
        {
            cert_file_path_ = cert_file_path;
        }

        // 建造私钥文件路径
        void build_key_file_path(const std::string &key_file_path)
        {
            key_file_path_ = key_file_path;
        }

        // 建造端口号
        void build_port(const uint16_t port)
        {
            port_ = port;
        }

        // 建造服务器名称
        void build_name(const std::string &name)
        {
            name_ = name;
        }

        // 是否使用SSL
        void build_use_ssl(const bool use_ssl)
        {
            use_ssl_ = use_ssl;
        }

        // 建造线程数
        void build_thread_num(const uint32_t thread_num)
        {
            thread_num_ = thread_num;
        }

        // 建造服务器选项
        void build_option(muduo::net::TcpServer::Option option)
        {
            option_ = option;
        }

        // 添加中间件
        void build_middleware(std::shared_ptr<zmiddleware::Middleware> middleware)
        {
            middlewares_.emplace_back(std::move(middleware));
        }

    protected:
        std::string cert_file_path_;                                                 // 证书文件路径
        std::string key_file_path_;                                                  // 私钥文件路径
        zssl::SslVersion version_ = zssl::SslVersion::TLS_1_2;                       // SSL版本
        uint16_t port_{};                                                            // 端口号
        std::string name_ = "HttpServer";                                            // 服务器名称
        bool use_ssl_ = false;                                                       // 是否使用SSL
        uint32_t thread_num_ = std::thread::hardware_concurrency();                  // 启动线程数
        muduo::net::TcpServer::Option option_ = muduo::net::TcpServer::kNoReusePort; // 服务器选项
        std::vector<std::shared_ptr<zmiddleware::Middleware>> middlewares_;          // 中间件列表
    };

    // HTTP服务器建造者
    class HttpServerBuilder final : public ServerBuilder
    {
    public:
        // 构建HTTP服务器
        std::unique_ptr<HttpServer> build()
        {
            if (use_ssl_ && (cert_file_path_.empty() || key_file_path_.empty()))
            {
                ZHTTP_LOG_FATAL("Certificate and key file paths must be set.");
                
                // 验证文件是否可读
                if (access(cert_file_path_.c_str(), R_OK) != 0)
                {
                    ZHTTP_LOG_FATAL("Cannot read certificate file: {}", cert_file_path_);
                }
                if (access(key_file_path_.c_str(), R_OK) != 0)
                {
                    ZHTTP_LOG_FATAL("Cannot read private key file: {}", key_file_path_);
                }
                return nullptr;
            }

            if (port_ == 0)
            {
                ZHTTP_LOG_FATAL("Port must be set.");
                return nullptr;
            }

            // 设置SSL
            auto &ssl_config = zhttp::zssl::SslConfig::get_instance();
            ssl_config.set_cert_file_path(cert_file_path_);
            ssl_config.set_key_file_path(key_file_path_);

            // 创建HTTP服务器实例
            auto server = std::make_unique<HttpServer>(port_, name_, use_ssl_, option_);
            server->set_thread_num(thread_num_);

            // 设置SSL上下文
            if (use_ssl_)
            {
                server->set_ssl_context();
            }

            // 添加中间件
            for (const auto &middleware : middlewares_)
            {
                server->add_middleware(middleware);
            }

            return server;
        }
    };
} // namespace zhttp
