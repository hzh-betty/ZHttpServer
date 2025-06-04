#include "http_server.h"
#include "http_context.h"
#include <muduo/base/Logging.h>
#include <utility>

namespace zhttp
{
    HttpServer::HttpServer(uint16_t port,
                           const std::string &name,
                           bool use_ssl,
                           muduo::net::TcpServer::Option option)
        : is_ssl_(use_ssl)
    {
        init(port, name, option);
    }

    // 启动线程数
    void HttpServer::set_thread_num(const uint32_t num) const
    {
        server_->setThreadNum(static_cast<int>(num));
    }

    // 启动
    void HttpServer::start()
    {
        LOG_INFO << "HttpServer[" << server_->name() << "] starts listening on " << server_->ipPort();
        if (is_ssl_)
        {
            set_ssl_context();
        }
        server_->start();
        main_loop_->loop();
    }

    // 注册静态路由回调
    void HttpServer::Get(const std::string &path, const HttpCallback &cb) const
    {
        router_->register_callback(path, HttpRequest::Method::GET, cb);
    }

    void HttpServer::Get(const std::string &path, zrouter::Router::HandlerPtr handler) const
    {
        router_->register_handler(path, HttpRequest::Method::GET, std::move(handler));
    }

    void HttpServer::Post(const std::string &path, const HttpCallback &cb) const
    {
        router_->register_callback(path, HttpRequest::Method::POST, cb);
    }

    void HttpServer::Post(const std::string &path, zrouter::Router::HandlerPtr handler) const
    {
        router_->register_handler(path, HttpRequest::Method::POST, std::move(handler));
    }

    // PUT 方法实现
    void HttpServer::Put(const std::string &path, const HttpCallback &cb) const
    {
        router_->register_callback(path, HttpRequest::Method::PUT, cb);
    }

    void HttpServer::Put(const std::string &path, zrouter::Router::HandlerPtr handler) const
    {
        router_->register_handler(path, HttpRequest::Method::PUT, std::move(handler));
    }

    // DELETE 方法实现
    void HttpServer::Delete(const std::string &path, const HttpCallback &cb) const
    {
        router_->register_callback(path, HttpRequest::Method::DELETE, cb);
    }

    void HttpServer::Delete(const std::string &path, zrouter::Router::HandlerPtr handler) const
    {
        router_->register_handler(path, HttpRequest::Method::DELETE, std::move(handler));
    }

    // PATCH 方法实现
    void HttpServer::Patch(const std::string &path, const HttpCallback &cb) const
    {
        router_->register_callback(path, HttpRequest::Method::PATCH, cb);
    }

    void HttpServer::Patch(const std::string &path, zrouter::Router::HandlerPtr handler) const
    {
        router_->register_handler(path, HttpRequest::Method::PATCH, std::move(handler));
    }

    // HEAD 方法实现
    void HttpServer::Head(const std::string &path, const HttpCallback &cb) const
    {
        router_->register_callback(path, HttpRequest::Method::HEAD, cb);
    }

    void HttpServer::Head(const std::string &path, zrouter::Router::HandlerPtr handler) const
    {
        router_->register_handler(path, HttpRequest::Method::HEAD, std::move(handler));
    }

    // OPTIONS 方法实现
    void HttpServer::Options(const HttpCallback &cb) const
    {
        router_->register_callback(options_path_, HttpRequest::Method::OPTIONS, cb);
    }

    void HttpServer::Options(zrouter::Router::HandlerPtr handler) const
    {
        router_->register_handler(options_path_, HttpRequest::Method::OPTIONS, std::move(handler));
    }


    // 注册动态路由回调
    void HttpServer::add_regex_route(HttpRequest::Method method, const std::string &path, const HttpCallback &cb) const
    {
        router_->register_regex_callback(path, method, cb);
    }

    void HttpServer::add_regex_route(HttpRequest::Method method, const std::string &path,
                                     zrouter::Router::HandlerPtr handler) const
    {
        router_->register_regex_handler(path, method, std::move(handler));
    }

    // 添加中间件
    void HttpServer::add_middleware(std::shared_ptr<zmiddleware::Middleware> middleware) const
    {
        middleware_chain_->add_middleware(std::move(middleware));
    }

    // 初始化
    void HttpServer::init(uint16_t port, const std::string &name, muduo::net::TcpServer::Option option)
    {
        // 初始化服务端元素
        listen_addr_ = std::make_unique<muduo::net::InetAddress>(port);
        main_loop_ = std::make_unique<muduo::net::EventLoop>();
        server_ = std::make_unique<muduo::net::TcpServer>
                (main_loop_.get(), *listen_addr_, name, option);
        router_ = std::make_unique<zrouter::Router>();
        middleware_chain_ = std::make_unique<zmiddleware::MiddlewareChain>();
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
        LOG_INFO << "HttpServer init successfully";
    }

    // 设置SSL上下文
    void HttpServer::set_ssl_context()
    {
        if (is_ssl_)
        {
            //  创建SSL上下文
            ssl_context_ = std::make_unique<zssl::SslContext>();
            if (!ssl_context_->init())
            {
                LOG_ERROR << "ssl context init failed";
                abort();
            }
        }
    }

    // 新链接建立回调
    void HttpServer::on_connection(const muduo::net::TcpConnectionPtr &conn)
    {
        LOG_INFO << "new connection accepted";
        if (conn->connected())
        {
            if (is_ssl_)
            {
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
            }
            conn->setContext(HttpContext()); // 为每个链接设置HttpContext
            LOG_INFO << "new connection accepted";
        }
        else
        {
            if (is_ssl_)
            {
                ssl_connections_.erase(conn); // 删除SSL连接
                LOG_INFO << "ssl connection closed";
            }
            else
            {
                LOG_INFO << "connection closed";
            }
        }
    }

    // 接受到数据回调
    void HttpServer::on_message(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf,
                                muduo::Timestamp receive_time)
    {
        auto *context = boost::any_cast<HttpContext>(conn->getMutableContext());
        if (!context->parse_request(buf, receive_time))
        {
            // 解析失败
            LOG_ERROR << "parse request failed";
            const std::string response = "HTTP/1.1 400 Bad Request\r\n\r\n";
            muduo::net::Buffer output;
            output.append(response);
            send(conn, output);
            conn->shutdown();
            return;
        }

        if (!context->is_parse_complete())
        {
            return;
        }

        on_request(conn, context->request());
        context->reset();
        LOG_INFO << "HttpServer on_message successfully";
    }

    // 得到一个完整的HTTP请求后的回调处理
    void HttpServer::on_request(const muduo::net::TcpConnectionPtr &conn,
                                const zhttp::HttpRequest &request)
    {
        const std::string &connection = request.get_header("Connection");
        // 判断是否需要关闭连接
        const bool close = connection == "close" ||
                           (request.get_version() == "1.0" && connection != "keep-alive");

        HttpResponse response;
        response.set_keep_alive(!close);

        // 判断是否为跨域请求（有 Origin 字段）
        const std::string &origin = request.get_header("Origin");
        response.set_request_origin(origin);

        callback_(request, &response);

        // 响应数据
        muduo::net::Buffer output;
        response.append_buffer(&output);
        LOG_INFO << "response: \n" << output.toStringPiece();

        send(conn, output);

        if (!response.is_keep_alive())
        {
            conn->shutdown();
        }
        LOG_INFO << "HttpServer handle HTTP request successfully";
    }

    // 中间件-路由-中间件处理
    void HttpServer::handle_request(const zhttp::HttpRequest &request,
                                    zhttp::HttpResponse *response) const
    {
        try
        {
            // 处理请求前中间件
            HttpRequest req = request;
            middleware_chain_->process_before(req);

            // 特殊处理 OPTIONS 请求
            if (req.get_method() == HttpRequest::Method::OPTIONS)
            {
                req.set_path(options_path_);
            }

            // 路由处理
            if (!router_->route(req, response))
            {
                response->set_status_code(zhttp::HttpResponse::StatusCode::NotFound);
                response->set_status_message("Not Found");
                response->set_body("404 Not Found");
                response->set_keep_alive(false);
            }

            // 处理请求后中间件
            middleware_chain_->process_after(*response);

            LOG_INFO << "HttpServer middleware-route-middleware successfully";
        } catch (const HttpResponse &req)
        {
            // 处理中间件抛出的响应（如CORS预检请求）
            *response = req;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "handle request error: " << e.what();
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
            LOG_WARN << "Connection not active, skip sending";
            return;
        }

        if (is_ssl_ && ssl_connections_.count(conn))
        {
            ssl_connections_[conn]->send(output.toStringPiece().data(), output.readableBytes());
        }
        else
        {
            conn->send(&output);
        }
        LOG_INFO << "HttpServer send successfully";
    }
} // namespace zhttp
