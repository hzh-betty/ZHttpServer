#pragma once
#include "middleware.h"

namespace zhttp::zmiddleware
{
    class MiddlewareChain
    {
    public:
        MiddlewareChain() = default;

        ~MiddlewareChain() = default;

        // 添加中间件
        void add_middleware(const std::shared_ptr<Middleware>& middleware);

        void process_before(HttpRequest& request);

        void process_after(HttpResponse& response);
    private:
        std::vector<std::shared_ptr<Middleware>> middlewares_; // 中间件链
    };
} // namespace zhttp::zmiddleware
