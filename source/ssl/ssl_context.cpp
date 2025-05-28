#include "ssl/ssl_context.h"
#include <muduo/base/Logging.h>
#include <openssl/err.h>

namespace zhttp::zssl
{
    SslContext::SslContext()
            : m_ctx_(nullptr)
    {

    }

    SslContext::~SslContext()
    {
        if (m_ctx_)
        {
            SSL_CTX_free(m_ctx_);
        }
    }

    // 初始化
    bool SslContext::SslContext::init()
    {
        // 初始化 OpenSSL
        OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS |
                         OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr);

        // 创建 SSL 上下文
        const SSL_METHOD *method = TLS_server_method();
        m_ctx_ = SSL_CTX_new(method);
        if (!m_ctx_)
        {
            handle_error("Failed to create SSL context");
            return false;
        }

        // 设置SSL选项
        long options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
                       SSL_OP_NO_COMPRESSION |
                       SSL_OP_CIPHER_SERVER_PREFERENCE;
        SSL_CTX_set_options(m_ctx_, options);

        // 加载证书与私钥
        if(!load_certificate())
        {
           return false;
        }

        // 设置协议版本
        if(!setup_protocal())
        {
            return false;
        }

        // 设置会话缓冲
        setup_session_cache();

        LOG_INFO << "SSL context initialized";
        return true;
    }

    SSL_CTX * SslContext::get_context()
    {
        return m_ctx_;
    }

    // 加载证书
    bool SslContext::load_certificate()
    {
        // 加载证书
        if (SSL_CTX_use_certificate_file(m_ctx_,
                                         config_.get_cert_file_path().c_str(), SSL_FILETYPE_PEM) <= 0)
        {
            handle_error("Failed to load server certificate");
            return false;
        }

        // 加载私钥
        if (SSL_CTX_use_PrivateKey_file(m_ctx_,
                                        config_.get_key_file_path().c_str(), SSL_FILETYPE_PEM) <= 0)
        {
            handle_error("Failed to load private key");
            return false;
        }

        // 验证私钥
        if (!SSL_CTX_check_private_key(m_ctx_))
        {
            handle_error("Private key does not match the certificate");
            return false;
        }

        // 加载证书链
        if (!config_.get_chain_file_path().empty())
        {
            if (SSL_CTX_use_certificate_chain_file(m_ctx_,
                                                   config_.get_chain_file_path().c_str()) <= 0)
            {
                handle_error("Failed to load certificate chain");
                return false;
            }
        }

        return true;
    }

    // 设置协议
    bool SslContext::setup_protocal()
    {
        // 设置 SSL/TLS 协议版本
        long options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3;
        switch (config_.get_version())
        {
            case SslVersion::TLS_1_0:
                options |= SSL_OP_NO_TLSv1;
                break;
            case SslVersion::TLS_1_1:
                options |= SSL_OP_NO_TLSv1_1;
                break;
            case SslVersion::TLS_1_2:
                options |= SSL_OP_NO_TLSv1_2;
                break;
            case SslVersion::TLS_1_3:
                options |= SSL_OP_NO_TLSv1_3;
                break;
        }
        SSL_CTX_set_options(m_ctx_, options);

        // 设置加密套件
        if (!config_.get_cipher_list().empty())
        {
            if (SSL_CTX_set_cipher_list(m_ctx_,
                                        config_.get_cipher_list().c_str()) <= 0)
            {
                handle_error("Failed to set cipher list");
                return false;
            }
        }

        return true;
    }

    // 设置会话缓冲
    void SslContext::setup_session_cache()
    {
        SSL_CTX_set_session_cache_mode(m_ctx_, SSL_SESS_CACHE_SERVER);
        SSL_CTX_sess_set_cache_size(m_ctx_, config_.get_session_cache_size());
        SSL_CTX_set_timeout(m_ctx_, config_.get_session_timeout());
    }

    void SslContext::handle_error(std::string_view msg)
    {
        char buf[256];
        ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
        LOG_ERROR << msg.data() << ": " << buf;
    }

} // namespace zhttp::zssl
