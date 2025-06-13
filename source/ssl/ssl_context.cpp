#include "ssl/ssl_context.h"
#include "log/http_logger.h"
#include <openssl/err.h>

namespace zhttp::zssl
{
    SslContext::SslContext()
        : m_ctx_(nullptr)
    {
        ZHTTP_LOG_DEBUG("SslContext constructed");
    }

    SslContext::~SslContext()
    {
        if (m_ctx_)
        {
            SSL_CTX_free(m_ctx_);
            ZHTTP_LOG_DEBUG("SslContext destroyed and SSL_CTX freed");
        }
    }

    // 初始化
    bool SslContext::init()
    {
        ZHTTP_LOG_INFO("Initializing SSL context...");
        // 初始化 OpenSSL
        OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS |
                         OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr);
        ZHTTP_LOG_DEBUG("OpenSSL library initialized");

        // 创建 SSL 上下文
        const SSL_METHOD *method = TLS_server_method();
        m_ctx_ = SSL_CTX_new(method);
        if (!m_ctx_)
        {
            handle_error("Failed to create SSL context");
            return false;
        }
        ZHTTP_LOG_DEBUG("SSL_CTX_new succeeded");

        // 设置SSL选项
        constexpr long options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
                                 SSL_OP_NO_COMPRESSION |
                                 SSL_OP_CIPHER_SERVER_PREFERENCE;
        SSL_CTX_set_options(m_ctx_, options);
        ZHTTP_LOG_DEBUG("SSL options set: NO_SSLv2, NO_SSLv3, NO_COMPRESSION, CIPHER_SERVER_PREFERENCE");

        // 加载证书与私钥
        if (!load_certificate())
        {
            ZHTTP_LOG_ERROR("Failed to load certificate or private key");
            return false;
        }

        // 设置协议版本
        if (!setup_protocal())
        {
            ZHTTP_LOG_ERROR("Failed to set protocol version");
            return false;
        }

        // 设置会话缓冲
        setup_session_cache();

        ZHTTP_LOG_INFO("SSL context initialized successfully");
        return true;
    }

    SSL_CTX *SslContext::get_context() const
    {
        return m_ctx_;
    }

    // 加载证书
    bool SslContext::load_certificate() const
    {
        ZHTTP_LOG_INFO("Loading certificate from: {}", config_.get_cert_file_path());
        if (SSL_CTX_use_certificate_file(m_ctx_,
                                         config_.get_cert_file_path().c_str(), SSL_FILETYPE_PEM) <= 0)
        {
            handle_error("Failed to load server certificate");
            return false;
        }
        ZHTTP_LOG_INFO("Server certificate loaded");

        ZHTTP_LOG_INFO("Loading private key from: {}", config_.get_key_file_path());
        if (SSL_CTX_use_PrivateKey_file(m_ctx_,
                                        config_.get_key_file_path().c_str(), SSL_FILETYPE_PEM) <= 0)
        {
            handle_error("Failed to load private key");
            return false;
        }
        ZHTTP_LOG_INFO("Private key loaded");

        if (!SSL_CTX_check_private_key(m_ctx_))
        {
            handle_error("Private key does not match the certificate");
            return false;
        }
        ZHTTP_LOG_INFO("Private key matches certificate");

        if (!config_.get_chain_file_path().empty())
        {
            ZHTTP_LOG_INFO("Loading certificate chain from: {}", config_.get_chain_file_path());
            if (SSL_CTX_use_certificate_chain_file(m_ctx_,
                                                   config_.get_chain_file_path().c_str()) <= 0)
            {
                handle_error("Failed to load certificate chain");
                return false;
            }
            ZHTTP_LOG_INFO("Certificate chain loaded");
        }
        else
        {
            ZHTTP_LOG_DEBUG("No certificate chain file specified");
        }

        return true;
    }

    // 设置协议
    bool SslContext::setup_protocal() const
    {
        ZHTTP_LOG_DEBUG("Setting SSL/TLS protocol version");
        long options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3;
        switch (config_.get_version())
        {
            case SslVersion::TLS_1_0:
                options |= SSL_OP_NO_TLSv1;
                ZHTTP_LOG_DEBUG("Disabling TLSv1.0");
                break;
            case SslVersion::TLS_1_1:
                options |= SSL_OP_NO_TLSv1_1;
                ZHTTP_LOG_DEBUG("Disabling TLSv1.1");
                break;
            case SslVersion::TLS_1_2:
                options |= SSL_OP_NO_TLSv1_2;
                ZHTTP_LOG_DEBUG("Disabling TLSv1.2");
                break;
            case SslVersion::TLS_1_3:
                options |= SSL_OP_NO_TLSv1_3;
                ZHTTP_LOG_DEBUG("Disabling TLSv1.3");
                break;
        }
        SSL_CTX_set_options(m_ctx_, options);

        if (!config_.get_cipher_list().empty())
        {
            ZHTTP_LOG_INFO("Setting cipher list: {}", config_.get_cipher_list());
            if (SSL_CTX_set_cipher_list(m_ctx_,
                                        config_.get_cipher_list().c_str()) <= 0)
            {
                handle_error("Failed to set cipher list");
                return false;
            }
            ZHTTP_LOG_INFO("Cipher list set");
        }
        else
        {
            ZHTTP_LOG_DEBUG("No cipher list specified, using default");
        }

        return true;
    }

    // 设置会话缓冲
    void SslContext::setup_session_cache() const
    {
        ZHTTP_LOG_DEBUG("Setting session cache: size={}, timeout={}",
                        config_.get_session_cache_size(), config_.get_session_timeout());
        SSL_CTX_set_session_cache_mode(m_ctx_, SSL_SESS_CACHE_SERVER);
        SSL_CTX_sess_set_cache_size(m_ctx_, config_.get_session_cache_size());
        SSL_CTX_set_timeout(m_ctx_, config_.get_session_timeout());
        ZHTTP_LOG_INFO("Session cache configured");
    }

    void SslContext::handle_error(std::string_view msg)
    {
        char buf[256];
        ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
        ZHTTP_LOG_ERROR("{}: {}", msg, buf);
    }
} // namespace zhttp::zssl
