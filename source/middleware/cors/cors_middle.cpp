#include "../../include/middleware/cors/cors_middle.h"
#include <muduo/base/Logging.h>
#include <algorithm>
#include <utility>

namespace zhttp::zmiddleware
{
    CorsMiddleware::CorsMiddleware(CorsConfig config) : config_(std::move(config)) {}

    // 请求前处理
    void CorsMiddleware::before(zhttp::HttpRequest &request)
    {
        LOG_DEBUG << "CorsMiddleware::before - Processing request";
        if (request.get_method() == zhttp::HttpRequest::Method::OPTIONS)
        {
            // 处理预检请求
            LOG_INFO << "Processing CORS preflight request";
            HttpResponse response;

            handle_preflight_request(request, response);
            throw response; // 抛出响应
        }
    }

    void CorsMiddleware::after(zhttp::HttpResponse &response)
    {
        LOG_DEBUG << "CorsMiddleware::after - Processing response";

        if (!config_.allow_origins_.empty())
        {
            // 如果允许所有源
            if (std::find(config_.allow_origins_.begin(), config_.allow_origins_.end(), "*") !=
                config_.allow_origins_.end())
            {
                response.set_header("Access-Control-Allow-Origin", "*");
            }
            else
            {
                // 否则设置允许的源
                response.set_header("Access-Control-Allow-Origin", config_.allow_origins_[0]);
            }
        }
        else
        {
            response.set_header("Access-Control-Allow-Origin", "*");
        }
    }

    // 将字符串数组连接成单个字符串
    std::string CorsMiddleware::join(const std::vector<std::string> &vec, const std::string &delimiter)
    {
        std::string result;
        for (size_t i = 0; i < vec.size(); ++i)
        {
            result += vec[i];
            if (i != vec.size() - 1)
            {
                result += delimiter;
            }
        }
        return result;
    }

    // 检查请求方法是否允许
    bool CorsMiddleware::is_origin_allowed(const std::string &origin)
    {
        return std::find(config_.allow_origins_.begin(), config_.allow_origins_.end(), origin)
               != config_.allow_origins_.end()
               || config_.allow_origins_.empty()
               || std::find(config_.allow_origins_.begin(),
                            config_.allow_origins_.end(), "*")
                  != config_.allow_origins_.end();
    }

    // 处理预检请求
    void CorsMiddleware::handle_preflight_request(HttpRequest &request, HttpResponse &response)
    {
        const std::string &origin = request.get_header("Origin");

        if (!is_origin_allowed(origin))
        {
            LOG_WARN << "Origin not allowed: " << origin;
            response.set_status_code(HttpResponse::StatusCode::Forbidden);
            return;
        }

        add_cors_headers(response, origin);
        response.set_status_code(HttpResponse::StatusCode::NoContent);
    }


    // 添加CORS头部
    void CorsMiddleware::add_cors_headers(HttpResponse &response, const std::string &origin) const
    {
        try
        {
            response.set_header("Access-Control-Allow-Origin", origin);

            // 设置凭证
            if (config_.allow_credentials)
            {
                response.set_header("Access-Control-Allow-Credentials", "true");
            }

            // 设置允许的请求头
            if (!config_.allow_methods_.empty())
            {
                response.set_header("Access-Control-Allow-Methods",
                                    join(config_.allow_methods_, ","));
            }

            // 最大缓存时间
            response.set_header("Access-Control-Max-Age",
                                std::to_string(config_.max_age));


            LOG_DEBUG << "CORS headers added: "
                      << "Access-Control-Allow-Origin: " << origin
                      << ", Access-Control-Allow-Methods: " << join(config_.allow_methods_, ",")
                      << ", Access-Control-Max-Age: " << config_.max_age;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "Error adding CORS headers: " << e.what();
        }
    }

} // namespace zhttp::zmiddleware