#include "../../include/middleware/middleware_chain.h"
#include "../../include/log/logger.h"

namespace zhttp::zmiddleware
{
    // 添加中间件
    void MiddlewareChain::add_middleware(std::shared_ptr<Middleware> &&middleware)
    {
        ZHTTP_LOG_DEBUG("Adding middleware to chain, total middlewares: {}", middlewares_.size());
        middlewares_.emplace_back(std::move(middleware));
        ZHTTP_LOG_INFO("Middleware added successfully, new total: {}", middlewares_.size());
    }

    // 处理请求中间件
    void MiddlewareChain::process_before(HttpRequest &request) const
    {
        ZHTTP_LOG_DEBUG("Starting before middleware processing, {} middlewares to process", middlewares_.size());
        
        // 遍历中间件
        for (size_t i = 0; i < middlewares_.size(); ++i)
        {
            auto& middleware = middlewares_[i];
            try
            {
                if (middleware)
                {
                    ZHTTP_LOG_DEBUG("Processing before middleware {}/{}", i + 1, middlewares_.size());
                    middleware->before(request);
                    ZHTTP_LOG_DEBUG("Before middleware {}/{} processed successfully", i + 1, middlewares_.size());
                }
                else
                {
                    ZHTTP_LOG_WARN("Null middleware encountered at position {}", i);
                }
            }
            catch (const std::exception &e)
            {
                ZHTTP_LOG_ERROR("Error in middleware {} before processing: {}", i, e.what());
                throw; // 重新抛出异常以停止处理链
            }
        }
        
        ZHTTP_LOG_DEBUG("All before middlewares processed successfully");
    }

    // 处理响应中间件
    void MiddlewareChain::process_after(HttpResponse &response)
    {
        ZHTTP_LOG_DEBUG("Starting after middleware processing, {} middlewares to process", middlewares_.size());
        
        // 反向处理响应，以保持中间件的正确执行顺序
        for (auto it = middlewares_.rbegin(); it != middlewares_.rend(); ++it)
        {
            size_t index = middlewares_.rend() - it - 1;
            try
            {
                if (*it)
                {
                    ZHTTP_LOG_DEBUG("Processing after middleware {}/{}", middlewares_.size() - index, middlewares_.size());
                    (*it)->after(response);
                    ZHTTP_LOG_DEBUG("After middleware {}/{} processed successfully", middlewares_.size() - index, middlewares_.size());
                }
                else
                {
                    ZHTTP_LOG_WARN("Null middleware encountered at reverse position {}", index);
                }
            }
            catch (const std::exception &e)
            {
                ZHTTP_LOG_ERROR("Error in middleware {} after processing: {}", index, e.what());
                // 继续处理其他中间件，不中断响应处理
            }
        }
        
        ZHTTP_LOG_DEBUG("All after middlewares processed successfully");
    }

} // zhttp::zmiddleware

