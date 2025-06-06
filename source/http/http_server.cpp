#include "http_server.h"
#include "http_context.h"
#include "../../include/log/logger.h"
#include <utility>

namespace zhttp
{
    HttpServer::HttpServer(uint16_t port,
                           const std::string &name,
                           bool use_ssl,
                           muduo::net::TcpServer::Option option)
        : is_ssl_(use_ssl)
    {
        ZHTTP_LOG_INFO("Creating HttpServer on port {}, name: {}, SSL: {}", 
                      port, name, use_ssl ? "enabled" : "disabled");
        init(port, name, option);
    }

    // 启动线程数
    void HttpServer::set_thread_num(const uint32_t num) const
    {
        ZHTTP_LOG_INFO("Setting thread number to {}", num);
        server_->setThreadNum(static_cast<int>(num));
    }

    // 启动
    void HttpServer::start() const
    {
        ZHTTP_LOG_INFO("HttpServer[{}] starts listening on {}", 
                      server_->name(), server_->ipPort());
        if (is_ssl_)
        {
            ZHTTP_LOG_INFO("SSL is enabled, setting up SSL context");
            //set_ssl_context();
        }
        server_->start();
        ZHTTP_LOG_INFO("Server started, entering event loop");
        main_loop_->loop();
    }

    // 注册静态路由回调
    void HttpServer::Get(const std::string &path, const HttpCallback &cb) const
    {
        ZHTTP_LOG_DEBUG("Registering GET route: {}", path);
        router_->register_callback(path, HttpRequest::Method::GET, cb);
    }

    void HttpServer::Get(const std::string &path, zrouter::Router::HandlerPtr handler) const
    {
        ZHTTP_LOG_DEBUG("Registering GET handler: {}", path);
        router_->register_handler(path, HttpRequest::Method::GET, std::move(handler));
    }

    void HttpServer::Post(const std::string &path, const HttpCallback &cb) const
    {
        ZHTTP_LOG_DEBUG("Registering POST route: {}", path);
        router_->register_callback(path, HttpRequest::Method::POST, cb);
    }

    void HttpServer::Post(const std::string &path, zrouter::Router::HandlerPtr handler) const
    {
        ZHTTP_LOG_DEBUG("Registering POST handler: {}", path);
        router_->register_handler(path, HttpRequest::Method::POST, std::move(handler));
    }

    // PUT 方法实现
    void HttpServer::Put(const std::string &path, const HttpCallback &cb) const
    {
        ZHTTP_LOG_DEBUG("Registering PUT route: {}", path);
        router_->register_callback(path, HttpRequest::Method::PUT, cb);
    }

    void HttpServer::Put(const std::string &path, zrouter::Router::HandlerPtr handler) const
    {
        ZHTTP_LOG_DEBUG("Registering PUT handler: {}", path);
        router_->register_handler(path, HttpRequest::Method::PUT, std::move(handler));
    }

    // DELETE 方法实现
    void HttpServer::Delete(const std::string &path, const HttpCallback &cb) const
    {
        ZHTTP_LOG_DEBUG("Registering DELETE route: {}", path);
        router_->register_callback(path, HttpRequest::Method::DELETE, cb);
    }

    void HttpServer::Delete(const std::string &path, zrouter::Router::HandlerPtr handler) const
    {
        ZHTTP_LOG_DEBUG("Registering DELETE handler: {}", path);
        router_->register_handler(path, HttpRequest::Method::DELETE, std::move(handler));
    }

    // PATCH 方法实现
    void HttpServer::Patch(const std::string &path, const HttpCallback &cb) const
    {
        ZHTTP_LOG_DEBUG("Registering PATCH route: {}", path);
        router_->register_callback(path, HttpRequest::Method::PATCH, cb);
    }

    void HttpServer::Patch(const std::string &path, zrouter::Router::HandlerPtr handler) const
    {
        ZHTTP_LOG_DEBUG("Registering PATCH handler: {}", path);
        router_->register_handler(path, HttpRequest::Method::PATCH, std::move(handler));
    }

    // HEAD 方法实现
    void HttpServer::Head(const std::string &path, const HttpCallback &cb) const
    {
        ZHTTP_LOG_DEBUG("Registering HEAD route: {}", path);
        router_->register_callback(path, HttpRequest::Method::HEAD, cb);
    }

    void HttpServer::Head(const std::string &path, zrouter::Router::HandlerPtr handler) const
    {
        ZHTTP_LOG_DEBUG("Registering HEAD handler: {}", path);
        router_->register_handler(path, HttpRequest::Method::HEAD, std::move(handler));
    }

    // OPTIONS 方法实现
    void HttpServer::Options(const HttpCallback &cb) const
    {
        ZHTTP_LOG_DEBUG("Registering OPTIONS route: {}", options_path_);
        router_->register_callback(options_path_, HttpRequest::Method::OPTIONS, cb);
    }

    void HttpServer::Options(zrouter::Router::HandlerPtr handler) const
    {
        ZHTTP_LOG_DEBUG("Registering OPTIONS handler: {}", options_path_);
        router_->register_handler(options_path_, HttpRequest::Method::OPTIONS, std::move(handler));
    }

    // 注册动态路由回调
    void HttpServer::add_regex_route(HttpRequest::Method method, const std::string &path, const HttpCallback &cb) const
    {
        ZHTTP_LOG_DEBUG("Registering regex route: method={}, path={}", static_cast<int>(method), path);
        router_->register_regex_callback(path, method, cb);
    }

    void HttpServer::add_regex_route(HttpRequest::Method method, const std::string &path,
                                     zrouter::Router::HandlerPtr handler) const
    {
        ZHTTP_LOG_DEBUG("Registering regex handler: method={}, path={}", static_cast<int>(method), path);
        router_->register_regex_handler(path, method, std::move(handler));
    }

    // 添加中间件
    void HttpServer::add_middleware(std::shared_ptr<zmiddleware::Middleware> middleware) const
    {
        ZHTTP_LOG_DEBUG("Adding middleware to chain");
        middleware_chain_->add_middleware(std::move(middleware));
    }

    // 初始化
    void HttpServer::init(uint16_t port, const std::string &name, muduo::net::TcpServer::Option option)
    {
        ZHTTP_LOG_DEBUG("Initializing HttpServer components");
        
        // 初始化服务端元素
        listen_addr_ = std::make_unique<muduo::net::InetAddress>(port);
        main_loop_ = std::make_unique<muduo::net::EventLoop>();
        server_ = std::make_unique<muduo::net::TcpServer>
                (main_loop_.get(), *listen_addr_, name, option);
        router_ = std::make_unique<zrouter::Router>();
        middleware_chain_ = std::make_unique<zmiddleware::MiddlewareChain>();
        
        ZHTTP_LOG_DEBUG("Server components initialized successfully");
        
        callback_ = [this](auto &&PH1, auto &&PH2)
        {
            handle_request(std::forward<decltype(PH1)>(PH1),
                           std::forward<decltype(PH2)>(PH2));
        };

        // 设置链接与数据回调
        server_->setConnectionCallback([this](auto &&PH1) { on_connection(std::forward<decltype(PH1)>(PH1)); });
        server_->setMessageCallback([this](auto &&PH1,
                                           auto &&PH2, auto &&PH3)
        {
            on_message(std::forward<decltype(PH1)>(PH1),
                       std::forward<decltype(PH2)>(PH2),
                       std::forward<decltype(PH3)>(PH3));
        });

        // 注册默认OPTIONS回调
        const HttpCallback default_options_callback = [&](const zhttp::HttpRequest &req, zhttp::HttpResponse *res)
        {
            res->set_response_line(req.get_version(),
                                   HttpResponse::StatusCode::NoContent, "No Content");
            res->set_header("Allow", "GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS");
        };
        router_->register_callback(options_path_,
                                   HttpRequest::Method::OPTIONS, default_options_callback);
        ZHTTP_LOG_INFO("HttpServer initialization completed successfully");
    }

    // 设置SSL上下文
    void HttpServer::set_ssl_context()
    {
        ZHTTP_LOG_INFO("Setting up SSL context");
        //  创建SSL上下文
        ssl_context_ = std::make_unique<zssl::SslContext>();
        if (!ssl_context_->init())
        {
            ZHTTP_LOG_ERROR("SSL context initialization failed");
            abort();
        }
        ZHTTP_LOG_INFO("SSL context setup completed");
    }

    // 新链接建立回调
    void HttpServer::on_connection(const muduo::net::TcpConnectionPtr &conn)
    {
        ZHTTP_LOG_DEBUG("Connection callback triggered for {}", conn->name());
        
        if (conn->connected())
        {
            ZHTTP_LOG_INFO("New connection established: {}", conn->peerAddress().toIpPort());
            
            if (is_ssl_)
            {
                ZHTTP_LOG_DEBUG("Creating SSL connection for {}", conn->name());
                // 创建SSL连接
                auto ssl_connection = std::make_unique<zssl::SslConnection>(conn,
                                                                            ssl_context_.get());
                ssl_connection->set_message_callback([this](auto &&PH1,
                                                            auto &&PH2, auto &&PH3)
                {
                    on_message(std::forward<decltype(PH1)>(PH1),
                               std::forward<decltype(PH2)>(PH2),
                               std::forward<decltype(PH3)>(PH3));
                });
                ssl_connections_[conn] = std::move(ssl_connection);
                ssl_connections_[conn]->handshake();
                ZHTTP_LOG_DEBUG("SSL handshake initiated for {}", conn->name());
            }
            conn->setContext(HttpContext()); // 为每个链接设置HttpContext
        }
        else
        {
            if (is_ssl_)
            {
                ssl_connections_.erase(conn); // 删除SSL连接
                ZHTTP_LOG_INFO("SSL connection closed: {}", conn->name());
            }
            else
            {
                ZHTTP_LOG_INFO("Connection closed: {}", conn->name());
            }
        }
    }

    // 接受到数据回调
    void HttpServer::on_message(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf,
                                muduo::Timestamp receive_time)
    {
        ZHTTP_LOG_DEBUG("Received message from {}, buffer size: {}", 
                       conn->name(), buf->readableBytes());
        
        auto *context = boost::any_cast<HttpContext>(conn->getMutableContext());
        if (!context->parse_request(buf, receive_time))
        {
            // 解析失败
            ZHTTP_LOG_ERROR("HTTP request parsing failed for connection {}", conn->name());
            const std::string response = "HTTP/1.1 400 Bad Request\r\n\r\n";
            muduo::net::Buffer output;
            output.append(response);
            send(conn, output);
            conn->shutdown();
            return;
        }

        if (!context->is_parse_complete())
        {
            ZHTTP_LOG_DEBUG("HTTP request parsing incomplete, waiting for more data from {}", 
                           conn->name());
            return;
        }

        ZHTTP_LOG_DEBUG("HTTP request parsing completed for {}", conn->name());
        on_request(conn, context->request());
        context->reset();
    }

    // 得到一个完整的HTTP请求后的回调处理
    void HttpServer::on_request(const muduo::net::TcpConnectionPtr &conn,
                                const zhttp::HttpRequest &request)
    {
        ZHTTP_LOG_INFO("Processing HTTP request: {} {} from {}", 
                      request.get_method_string(),
                      request.get_path(),
                      conn->peerAddress().toIpPort());
        
        const std::string &connection = request.get_header("Connection");
        // 判断是否需要关闭连接
        const bool close = connection == "close" ||
                           (request.get_version() == "1.0" && connection != "keep-alive");

        ZHTTP_LOG_DEBUG("Connection keep-alive: {}", close ? "false" : "true");

        HttpResponse response;
        response.set_keep_alive(!close);

        // 判断是否为跨域请求（有 Origin 字段）
        const std::string &origin = request.get_header("Origin");
        if (!origin.empty()) {
            ZHTTP_LOG_DEBUG("CORS request detected, origin: {}", origin);
        }
        response.set_request_origin(origin);

        callback_(request, &response);

        // 响应数据
        muduo::net::Buffer output;
        response.append_buffer(&output);
        ZHTTP_LOG_DEBUG("Sending response to {}, status: {}", 
                       conn->name(), static_cast<int>(response.get_status_code()));

        send(conn, output);

        if (!response.is_keep_alive())
        {
            ZHTTP_LOG_DEBUG("Closing connection {}", conn->name());
            conn->shutdown();
        }
    }

    // 中间件-路由-中间件处理
    void HttpServer::handle_request(const zhttp::HttpRequest &request,
                                    zhttp::HttpResponse *response) const
    {
        try
        {
            ZHTTP_LOG_DEBUG("Starting middleware-route-middleware processing");
            
            // 处理请求前中间件
            HttpRequest req = request;
            middleware_chain_->process_before(req);
            ZHTTP_LOG_DEBUG("Before middleware processing completed");

            // 特殊处理 OPTIONS 请求
            if (req.get_method() == HttpRequest::Method::OPTIONS)
            {
                ZHTTP_LOG_DEBUG("Processing OPTIONS request");
                req.set_path(options_path_);
            }

            // 路由处理
            if (!router_->route(req, response))
            {
                ZHTTP_LOG_WARN("Route not found: {} {}", 
                              req.get_method_string(), req.get_path());
                response->set_status_code(zhttp::HttpResponse::StatusCode::NotFound);
                response->set_status_message("Not Found");
                response->set_body("404 Not Found");
                response->set_keep_alive(false);
            }
            else
            {
                ZHTTP_LOG_DEBUG("Route processed successfully");
            }

            // 处理请求后中间件
            middleware_chain_->process_after(*response);
            ZHTTP_LOG_DEBUG("After middleware processing completed");
        }
        catch (const HttpResponse &req)
        {
            // 处理中间件抛出的响应（如CORS预检请求）
            ZHTTP_LOG_DEBUG("Middleware threw HttpResponse, using it as final response");
            *response = req;
        }
        catch (const std::exception &e)
        {
            ZHTTP_LOG_ERROR("Exception in request handling: {}", e.what());
            response->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
            response->set_status_message("Internal Server Error");
            response->set_body(e.what());
        }

        response->set_version(request.get_version()); // 设置响应版本号
        response->set_header("Date",
                             zhttp::HttpResponse::to_http_date(muduo::Timestamp::now()));
    }

    // 向客户端发送数据
    void HttpServer::send(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer &output)
    {
        if (!conn->connected())
        {
            ZHTTP_LOG_WARN("Connection {} not active, skip sending", conn->name());
            return;
        }

        ZHTTP_LOG_DEBUG("Sending {} bytes to {}", output.readableBytes(), conn->name());

        if (is_ssl_ && ssl_connections_.count(conn))
        {
            ssl_connections_[conn]->send(output.toStringPiece().data(), output.readableBytes());
            ZHTTP_LOG_DEBUG("Data sent via SSL connection");
        }
        else
        {
            conn->send(&output);
            ZHTTP_LOG_DEBUG("Data sent via regular connection");
        }
    }
} // namespace zhttp
