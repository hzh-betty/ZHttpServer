#include "middleware/cors/cors_middle.h"
#include <muduo/base/Logging.h>
#include <algorithm>
#include <utility>

namespace zhttp::zmiddleware
{
    CorsMiddleware::CorsMiddleware(CorsConfig config) : config_(std::move(config)) {}

    // 请求前处理
    void CorsMiddleware::before(zhttp::HttpRequest &request)
    {
        LOG_DEBUG << "Processing request";
        // 判断是否为跨域请求（有 Origin 字段）
        const std::string &origin = request.get_header("Origin");
        bool is_cors_request = !origin.empty() && config_.server_origin_ != origin;

        if (request.get_method() == HttpRequest::Method::OPTIONS && is_cors_request)
        {
            // 仅处理跨域的 OPTIONS 预检请求
            HttpResponse response;
            handle_preflight_request(request, response);
            throw response;
        }

        // 同源请求或非跨域预请求：无需额外处理，继续执行后续中间件
    }

    void CorsMiddleware::after(zhttp::HttpResponse &response)
    {
        LOG_DEBUG << "CorsMiddleware::after - Processing response";

        // 判断是否为跨域请求（有 Origin 字段）
        const std::string &origin = response.get_request_origin();
        bool is_cors_request = !origin.empty() && config_.server_origin_ != origin;

        if(!is_cors_request)
            return;

        if (!config_.allow_origins_.empty())
        {
            // 如果允许所有源
            if (std::find(config_.allow_origins_.begin(), config_.allow_origins_.end(), "*") !=
                config_.allow_origins_.end())
            {
                add_cors_headers(response, "*");
            }
            else
            {
                // 否则设置允许的源
                add_cors_headers(response, config_.allow_origins_[0]);
            }
        }
        else
        {
            // 如果没有允许的源，则允许所有源
            add_cors_headers(response, "*");
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

    // 检查请求源是否允许进行跨域请求
    bool CorsMiddleware::is_origin_allowed(const std::string &origin)
    {
        // 如果没有允许的源，则允许所有源
        if (config_.allow_origins_.empty()) return true;

        if (std::find(config_.allow_origins_.begin(), config_.allow_origins_.end(), "*")
            != config_.allow_origins_.end())
        {
            return true;
        }

        // 检查请求源是否在允许的源列表中
        return std::find(config_.allow_origins_.begin(), config_.allow_origins_.end(), origin)
               != config_.allow_origins_.end();
    }

    // 处理预检请求
    void CorsMiddleware::handle_preflight_request(const HttpRequest &request, HttpResponse &response)
    {
        const auto origin = request.get_header("Origin");
        if (!is_origin_allowed(origin))
        {
            LOG_WARN << "CORS preflight blocked for origin: " << origin;
            response.set_response_line(request.get_version(),
                                  HttpResponse::StatusCode::Forbidden,
                                  "Forbidden");
            return;
        }
        add_cors_headers(response, origin);
        response.set_response_line(request.get_version(),
                              HttpResponse::StatusCode::NoContent,
                              "No Content");
        LOG_INFO << "CORS preflight OK for origin: " << origin;
    }


    // 添加CORS头部
    void CorsMiddleware::add_cors_headers(HttpResponse &response, const std::string &origin) const
    {
        // 设置允许的源
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

        // 设置接受的请求头
        if (!config_.allow_headers_.empty())
        {
            response.set_header("Access-Control-Allow-Headers",
                                join(config_.allow_headers_, ","));
        }

        // 最大缓存时间
        response.set_header("Access-Control-Max-Age",
                            std::to_string(config_.max_age));

        LOG_DEBUG << "CORS headers added: "
                  << "Access-Control-Allow-Origin: " << origin
                  << ", Access-Control-Allow-Methods: " << join(config_.allow_methods_, ",")
                  << ", Access-Control-Max-Age: " << config_.max_age;

    }

} // namespace zhttp::zmiddleware