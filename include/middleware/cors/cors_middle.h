#pragma once
#include "../middleware.h"
#include "../../http/http_request.h"
#include "../../http/http_response.h"
#include "cors_config.h"

namespace zhttp::zmiddleware
{
    class CorsMiddleware : public Middleware
    {
    public:
        explicit CorsMiddleware(CorsConfig config = CorsConfig::default_config());

        // 请求前处理
        void before(HttpRequest& request)override;

        // 响应后处理
        void after(HttpResponse& response)override;

        // 处理OPTIONS请求
        static std::string join(const std::vector<std::string>& vec, const std::string& delimiter);

        ~CorsMiddleware() override = default;
    private:

        // 检查请求源是否允许
        bool is_origin_allowed(const std::string&origin);

        // 处理预检请求
        void handle_preflight_request(HttpRequest& request, HttpResponse& response) ;

        // 添加CORS头部
        void add_cors_headers(HttpResponse& response,const std::string & origin) const;

    protected:
        CorsConfig config_;
        bool is_cors_request_; // 是否是CORS请求
    };
} // namespace zhttp::zmiddleware
