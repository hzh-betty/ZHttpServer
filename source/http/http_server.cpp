#include "http_server.h"

#include <utility>

namespace zhttp
{
    HttpServer::HttpServer(uint16_t port,
                           const std::string &name,
                           bool use_ssl,
                           muduo::net::TcpServer::Option option)
            : listen_addr_(port), server_(&main_loop_, listen_addr_, name, option), is_ssl_(use_ssl), callback_(
            [this](auto &&PH1, auto &&PH2)
            {
                handle_request(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
            })
    {
        init();
    }

    // 启动线程数
    void HttpServer::set_thread_num(uint32_t num)
    {
        server_.setThreadNum((int) num);
    }

    // 启动
    void HttpServer::start()
    {
        LOG_INFO << "HttpServer[" << server_.name() << "] starts listening on " << server_.ipPort();
        server_.start();
        main_loop_.loop();
    }

    // 获取主线程loop
    muduo::net::EventLoop *HttpServer::get_main_loop()
    {
        return &main_loop_;
    }

    // 注册静态路由回调
    void HttpServer::Get(const std::string &path, const HttpCallback &cb)
    {
        router_.register_callback(path, HttpRequest::Method::GET, cb);
    }

    void HttpServer::Get(const std::string &path, zrouter::Router::HandlerPtr handler)
    {
        router_.register_handler(path, HttpRequest::Method::GET, std::move(handler));
    }

    void HttpServer::Post(const std::string &path, const HttpCallback &cb)
    {
        router_.register_callback(path, HttpRequest::Method::POST, cb);
    }

    void HttpServer::Post(const std::string &path, zrouter::Router::HandlerPtr handler)
    {
        router_.register_handler(path, HttpRequest::Method::POST, std::move(handler));
    }

    // 注册动态路由回调
    void HttpServer::add_regex_route(HttpRequest::Method method, const std::string &path, const HttpCallback &cb)
    {
        router_.register_regex_callback(path, method, cb);
    }


    void HttpServer::add_regex_route(HttpRequest::Method method, const std::string &path,
                                     zrouter::Router::HandlerPtr handler)
    {
        router_.register_regex_handler(path, method, std::move(handler));
    }

    // 添加中间件
    void HttpServer::add_middleware(const std::shared_ptr<zmiddleware::Middleware> &middleware)
    {
        middleware_chain_.add_middleware(middleware);
    }

    // 启动SSL
    void HttpServer::enable_ssl(bool enable)
    {
        is_ssl_ = enable;
    }

    // 初始化
    void HttpServer::init()
    {
        server_.setConnectionCallback([this](auto &&PH1)
        { on_connection(std::forward<decltype(PH1)>(PH1)); });
        server_.setMessageCallback([this](auto &&PH1,
                                          auto &&PH2, auto &&PH3)
                                   {
                                       on_message(std::forward<decltype(PH1)>(PH1),
                                                  std::forward<decltype(PH2)>(PH2),
                                                          std::forward<decltype(PH3)>(PH3));
                                   });
        LOG_INFO  << "HttpServer init";
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
        if (conn->connected())
        {
            if (is_ssl_)
            {
                // 创建SSL连接
                auto ssl_connection = std::make_unique<zssl::SslConnection>(conn,
                                                                            ssl_context_.get());
                ssl_connection->set_message_callback([this](auto && PH1,
                        auto && PH2, auto && PH3)
                        { on_message(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3)); });
                ssl_connections_[conn] = std::move(ssl_connection);
                ssl_connections_[conn]->handshake();
            }
            conn->setContext(HttpContext()); // 为每个链接设置HttpContext
        }
        else
        {
            if (is_ssl_)
            {
                ssl_connections_.erase(conn); // 删除SSL连接
                LOG_INFO << "ssl connection closed";
            }
        }
    }

    // 接受到数据回调
    void HttpServer::on_message(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf,
                                muduo::Timestamp receive_time)
    {
        if (is_ssl_)
        {
            LOG_INFO << "on_message: ssl connection receive data";
            // 1. 查找SSL连接
            auto iter = ssl_connections_.find(conn);

            if (iter == ssl_connections_.end())
            {
                LOG_ERROR << "ssl connection not found";
                return;
            }

            if (iter != ssl_connections_.end())
            {
                // 2. 处理SSL连接
                LOG_INFO  << "iter != ssl_connections_.end()";
                iter->second->on_read(conn, buf, receive_time);

                // 3. 如果握手未完成
                if (!iter->second->is_handshake_completed())
                {
                    LOG_INFO << "ssl handshake not completed";
                    return;
                }

//                // 4. 获取解密后的数据
//                muduo::net::Buffer *decrypted_buffer = iter->second->get_decrypted_buffer();
//                if (decrypted_buffer->readableBytes() == 0) return;
//
//                // 5. 使用解密后的数据进行处理
//                buf = decrypted_buffer;
            }

        }
        auto *context = boost::any_cast<HttpContext>(conn->getMutableContext());
        if (!context->parse_request(buf, receive_time))
        {
            // 解析失败
            conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
            conn->shutdown();
            return;
        }
        if (context->is_parse_complete())
        {
            on_request(conn, context->request());
            context->reset();
        }
        LOG_INFO  << "HttpServer on_message";
    }

    // 得到一个完整的HTTP请求后的回调处理
    void HttpServer::on_request(const muduo::net::TcpConnectionPtr &conn,
                                zhttp::HttpRequest &request)
    {
        const std::string &connection = request.get_header("Connection");
        // 判断是否需要关闭连接
        bool close = connection == "close" ||
                     (request.get_version() == "1.0" && connection != "keep-alive");

        HttpResponse response;
        response.set_keep_alive(!close);

        callback_(request, &response);

        // 响应数据
        muduo::net::Buffer output;
        response.append_buffer(&output);
        LOG_INFO << "response: " << output.toStringPiece();

        conn->send(&output);

        if (!response.is_keep_alive())
        {
            conn->shutdown();
        }
        LOG_INFO  << "HttpServer on_request";
    }

    // 中间件-路由-中间件处理
    void HttpServer::handle_request(const zhttp::HttpRequest &request,
                                    zhttp::HttpResponse *response)
    {
        try
        {

            // 处理请求前中间件
            HttpRequest req = request;
            middleware_chain_.process_before(req);

            // 路由处理
            if (!router_.route(req, response))
            {
                response->set_status_code(zhttp::HttpResponse::StatusCode::NotFound);
                response->set_status_message("Not Found");
                response->set_keep_alive(false);
            }

            // 处理请求后中间件
            middleware_chain_.process_after(*response);
        }
        catch (const HttpResponse &req)
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

        LOG_INFO  << "HttpServer handle_request";
    }

} // namespace zhttp