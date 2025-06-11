#pragma once

#include "../http/http_request.h"
#include "../http/http_response.h"

namespace zhttp::zrouter
{
    class RouterHandler
    {
    public:
        virtual ~RouterHandler() = default;

        // 处理请求
        virtual void handle_request(const HttpRequest &request, HttpResponse *response) = 0;
    };
}// namespace router

