#include <muduo/base/Logging.h>
#include "include/http/http_server.h"
#include "include/ssl/ssl_config.h"

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
        std::string certFile = "/home/betty/server.crt";
        std::string keyFile = "/home/betty/server.key";

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
        serverPtr->Get("/", [](const zhttp::HttpRequest& req, zhttp::HttpResponse* resp) {
            resp->set_status_code(zhttp::HttpResponse::StatusCode::OK);
            resp->set_status_message("OK");
            resp->set_content_type("text/plain");
            resp->set_body("Hello, World!");
        });

        // 启动服务器
        LOG_INFO << "Server starting on port 443...";
        serverPtr->start();
    }
    catch (const std::exception& e)
    {
        LOG_FATAL << "Server start failed: " << e.what();
        return 1;
    }

    return 0;
}