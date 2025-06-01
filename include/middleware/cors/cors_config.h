#pragma once
#include <string>
#include <vector>

namespace zhttp::zmiddleware
{
    struct CorsConfig
    {
        std::vector<std::string> allow_origins_; // 允许的源
        std::vector<std::string> allow_methods_; // 允许的方法
        std::vector<std::string> allow_headers_; // 允许的请求头
        std::string server_origin_; // 服务器源地址
        bool allow_credentials = false; // 是否允许凭证
        int max_age = 0; // 预检请求的最大有效期

        static CorsConfig default_config()
        {
            CorsConfig config;
            config.allow_origins_ = {"*"}; // 默认允许所有源
            config.allow_methods_ = {"GET", "POST", "PUT", "DELETE", "OPTIONS"}; // 默认允许所有方法
            config.allow_headers_ = {"Content-Type", "Authorization"}; // 默认允许的请求头
            config.server_origin_ = "https://1.95.159.45";
            return config;
        }
    };
} // namespace zhttp::zmiddleware