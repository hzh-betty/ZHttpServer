#include "../../include/middleware/middleware_chain.h"
#include <muduo/base/Logging.h>

namespace zhttp::zmiddleware
{
    // 添加中间件
    void MiddlewareChain::add_middleware(std::shared_ptr<Middleware> &&middleware)
    {
        middlewares_.emplace_back(std::move(middleware));
    }

    // 处理请求中间件
    void MiddlewareChain::process_before(HttpRequest &request) const
    {
        // 遍历中间件
        for (auto & middleware : middlewares_)
        {
            try
            {
                if (middleware)
                {
                    middleware->before(request);
                }
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Error in middleware begin processing: " << e.what();
            }
        }
    }

    // 处理响应中间件
    void MiddlewareChain::process_after(HttpResponse &response)
    {
        // 反向处理响应，以保持中间件的正确执行顺序
        for (auto it = middlewares_.rbegin(); it != middlewares_.rend(); ++it)
        {
            try
            {
                if (*it)
                {
                    (*it)->after(response);
                }
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Error in middleware after processing: " << e.what();
            }
        }

    }

} // zhttp::zmiddleware

