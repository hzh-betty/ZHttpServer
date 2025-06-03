#pragma once

#include "ssl_config.h"
#include <openssl/ssl.h>
#include <memory>
#include <string_view>

namespace zhttp::zssl
{
    class SslContext
    {
    public:
        SslContext();

        ~SslContext();

        // 初始化
        bool init();

        SSL_CTX *get_context() const;
    private:
        // 加载证书
        bool load_certificate() const;

        // 设置协议
        bool setup_protocal() const;

        // 设置会话缓冲
        void setup_session_cache() const;

        static void handle_error(std::string_view msg);
    private:
        SSL_CTX *m_ctx_; // SSL上下文
        SslConfig& config_ = SslConfig::get_instance();
    };
} // namespace zhttp::zssl