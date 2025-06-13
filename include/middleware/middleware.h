#pragma once
#include "http/http_request.h"
#include "http/http_response.h"
#include <memory>

namespace zhttp::zmiddleware
{
    class Middleware
    {
    public:
        virtual ~Middleware() = default;

        // 请求前处理
        virtual void before(HttpRequest &request) = 0;

        // 响应后处理
        virtual void after(HttpResponse &response) = 0;

        // 设置下一个中间件
        void set_next(std::shared_ptr<Middleware> &&next)
        {
            next_middleware_ = std::move(next);
        }

    protected:
        std::shared_ptr<Middleware> next_middleware_; // 下一个中间件
    };

} // namespace zhttp::zmiddleware
