#include <muduo/base/Logging.h>
#include "include/http/http_server.h"
#include "include/ssl/ssl_config.h"
#include "include/middleware/cors/cors_middle.h"

int main()
{
    // 设置日志级别
    muduo::Logger::setLogLevel(muduo::Logger::INFO);

    try
    {
        // 创建服务器实例
        auto serverPtr = std::make_unique<zhttp::HttpServer>(443, "HTTPServer", true);
        // 加载 SSL 配置
        auto &sslConfig = zhttp::zssl::SslConfig::get_instance();

        // 获取当前工作目录
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != nullptr)
        {
            LOG_INFO << "Current working directory: " << cwd;
        }

        // 设置证书文件（使用绝对路径）
        const std::string certFile = "/home/betty/ssl/server.crt";
        const std::string keyFile = "/home/betty/ssl/server.key";

        LOG_INFO << "Loading certificate from: " << certFile;
        LOG_INFO << "Loading private key from: " << keyFile;

        sslConfig.set_cert_file_path(certFile);
        sslConfig.set_key_file_path(keyFile);
        sslConfig.set_version(zhttp::zssl::SslVersion::TLS_1_2);

        // 验证文件是否可读
        if (access(certFile.c_str(), R_OK) != 0)
        {
            LOG_FATAL << "Cannot read certificate file: " << certFile;
            return 1;
        }
        if (access(keyFile.c_str(), R_OK) != 0)
        {
            LOG_FATAL << "Cannot read private key file: " << keyFile;
            return 1;
        }

        // 设置线程数
        serverPtr->set_thread_num(4);

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
            resp->set_header("Location","https://example.com");
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

        serverPtr->add_middleware(std::make_shared<zhttp::zmiddleware::CorsMiddleware>
                                          (zhttp::zmiddleware::CorsConfig::default_config()));

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