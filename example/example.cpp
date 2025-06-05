#include <muduo/base/Logging.h>
#include "../include/http/http_server.h"
#include "../include/middleware/cors/cors_middle.h"

int main()
{
    // 设置日志级别
    muduo::Logger::setLogLevel(muduo::Logger::INFO);

    try
    {
        const auto builder = std::make_unique<zhttp::HttpServerBuilder>();
        builder->build_cert_file_path("/home/betty/ssl/server.crt");
        builder->build_key_file_path("/home/betty/ssl/server.key");
        builder->build_port(8080);
        builder->build_name("HttpServer");
        builder->build_use_ssl(true);
        builder->build_thread_num(4);
        builder->build_middleware(zhttp::zmiddleware::MiddlewareFactory::create<zhttp::zmiddleware::CorsMiddleware>());
        auto serverPtr = builder->build();

        // 添加一个测试端点
        serverPtr->Get("/get", [](const zhttp::HttpRequest &req, zhttp::HttpResponse *resp)
        {
            resp->set_response_line(req.get_version(),
                                    zhttp::HttpResponse::StatusCode::OK, "OK");
            resp->set_content_type("text/plain");
            resp->set_body("Hello, World!");
        });

        serverPtr->Post("/post", [](const zhttp::HttpRequest &req, zhttp::HttpResponse *resp)
        {
            resp->set_response_line(req.get_version(),
                                    zhttp::HttpResponse::StatusCode::Created, "Created");
            resp->set_header("Location", "https://example.com");
            resp->set_content_type("application/json");
            resp->set_body(R"({"message":"POST request processed"})");
        });

        // PUT请求示例
        serverPtr->Put("/update", [](const zhttp::HttpRequest &req, zhttp::HttpResponse *resp)
        {
            resp->set_response_line(req.get_version(),
                                    zhttp::HttpResponse::StatusCode::OK, "OK");
            resp->set_content_type("application/json");
            resp->set_body(R"({"message":"PUT request processed"})");
        });

        // DELETE请求示例
        serverPtr->Delete("/delete", [](const zhttp::HttpRequest &req, zhttp::HttpResponse *resp)
        {
            resp->set_response_line(req.get_version(),
                                    zhttp::HttpResponse::StatusCode::OK, "OK");
            resp->set_content_type("application/json");
            resp->set_body(R"({"message":"DELETE request processed"})");
        });

        // PATCH请求示例
        serverPtr->Patch("/patch", [](const zhttp::HttpRequest &req, zhttp::HttpResponse *resp)
        {
            resp->set_response_line(req.get_version(),
                                    zhttp::HttpResponse::StatusCode::OK, "OK");
            resp->set_content_type("application/json");
            resp->set_body(R"({"message":"PATCH request processed"})");
        });

        // HEAD请求示例
        serverPtr->Head("/head", [](const zhttp::HttpRequest &req, zhttp::HttpResponse *resp)
        {
            resp->set_response_line(req.get_version(),
                                    zhttp::HttpResponse::StatusCode::NoContent, "No Content");
            resp->set_header("Content-Length", "0");
        });

        // OPTIONS请求（已默认注册，可自定义）
        serverPtr->Options([](const zhttp::HttpRequest &req, zhttp::HttpResponse *resp)
        {
            resp->set_response_line(req.get_version(),
                                    zhttp::HttpResponse::StatusCode::OK, "OK");
            resp->set_header("Allow", "GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS");
            resp->set_content_type("text/plain");
            resp->set_body("Supported methods");
        });


        // 启动服务器
        LOG_INFO << "Server starting on port 443...";
        serverPtr->start();
    }
    catch (const std::exception &e)
    {
        LOG_FATAL << "Server start failed: " << e.what();
        return 1;
    }

    return 0;
}
