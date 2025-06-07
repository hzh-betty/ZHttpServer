#include "../../ssl/ssl_connection.h"
#include "../../include/log/logger.h"
#include <openssl/err.h>

#include <utility>

namespace zhttp::zssl
{
    // 自定义 BIO 方法
    static BIO_METHOD *create_custom_bio_method()
    {
        BIO_METHOD *method = BIO_meth_new(BIO_TYPE_MEM, "custom");
        BIO_meth_set_write(method, SslConnection::bio_write);
        BIO_meth_set_read(method, SslConnection::bio_read);
        BIO_meth_set_ctrl(method, SslConnection::bio_ctrl);
        return method;
    }

    SslConnection::SslConnection(muduo::net::TcpConnectionPtr conn, SslContext *ctx)
        : ssl_(nullptr), context_(ctx), connection_(std::move(conn)), state_(SslState::HANDSHAKE),
          read_bio_(nullptr),
          write_bio_(nullptr), message_callback_(nullptr)
    {
        ZHTTP_LOG_DEBUG("SslConnection: constructing for peer {}", connection_->peerAddress().toIpPort());
        ssl_ = SSL_new(context_->get_context());
        if (!ssl_)
        {
            ZHTTP_LOG_ERROR("SslConnection: Failed to create SSL object: {}", ERR_error_string(ERR_get_error(), nullptr));
            return;
        }

        read_bio_ = BIO_new(BIO_s_mem());
        write_bio_ = BIO_new(BIO_s_mem());

        if (!read_bio_ || !write_bio_)
        {
            ZHTTP_LOG_ERROR("SslConnection: Failed to create BIO objects for peer {}", connection_->peerAddress().toIpPort());
            SSL_free(ssl_);
            ssl_ = nullptr;
            return;
        }

        SSL_set_bio(ssl_, read_bio_, write_bio_);
        SSL_set_accept_state(ssl_);
        SSL_set_mode(ssl_, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
        SSL_set_mode(ssl_, SSL_MODE_ENABLE_PARTIAL_WRITE);

        connection_->setMessageCallback(
            [this](auto &&PH1, auto &&PH2, auto &&PH3)
            {
                on_read(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
                        std::forward<decltype(PH3)>(PH3));
            });

        ZHTTP_LOG_INFO("SslConnection: SSL connection created for peer {}", connection_->peerAddress().toIpPort());
    }

    SslConnection::~SslConnection()
    {
        ZHTTP_LOG_DEBUG("SslConnection: destructing for peer {}", connection_->peerAddress().toIpPort());
        if (ssl_)
        {
            SSL_free(ssl_);
        }
    }

    // 处理 SSL 握手
    void SslConnection::handshake()
    {
        ZHTTP_LOG_DEBUG("SslConnection: handshake start for peer {}", connection_->peerAddress().toIpPort());
        SSL_set_accept_state(ssl_);
        handle_handshake();
    }

    // BIO 方法
    void SslConnection::send(const void *data, size_t len)
    {
        if (state_ != SslState::ESTABLISHED)
        {
            ZHTTP_LOG_ERROR("SslConnection: Cannot send data before handshake for peer {}", connection_->peerAddress().toIpPort());
            return;
        }
        ZHTTP_LOG_DEBUG("SslConnection: sending {} bytes (will encrypt) to peer {}", len, connection_->peerAddress().toIpPort());
        on_encrypted(data, len);
        drain_write_bio();
    }

    void SslConnection::on_read(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf,
                                const muduo::Timestamp &time)
    {
        // 1. 写入所有接收到的加密数据到读 BIO
        receive_time_ = time;
        if (const int n = static_cast<int>(buf->readableBytes()); n > 0)
        {
            ZHTTP_LOG_DEBUG("SslConnection: received {} bytes encrypted data from peer {}", n, conn->peerAddress().toIpPort());
            BIO_write(read_bio_, buf->peek(), n);
            buf->retrieve(n);
        }

        // 2. 握手阶段
        if (state_ == SslState::HANDSHAKE)
        {
            ZHTTP_LOG_DEBUG("SslConnection: in handshake state, handling handshake for peer {}", conn->peerAddress().toIpPort());
            handle_handshake();
            return;
        }

        if (state_ != SslState::ESTABLISHED)
        {
            ZHTTP_LOG_ERROR("SslConnection: handshake failed for peer {}", conn->peerAddress().toIpPort());
            return;
        }

        // 3. 解密报文，并向回调给上层on_message处理
        on_decrypted();
        if (message_callback_ && decrypted_buffer_.readableBytes() > 0)
        {
            ZHTTP_LOG_DEBUG("SslConnection: invoking message callback with {} bytes decrypted data for peer {}", decrypted_buffer_.readableBytes(), conn->peerAddress().toIpPort());
            message_callback_(connection_, &decrypted_buffer_, receive_time_);
        }
    }

    void SslConnection::handle_handshake()
    {
        const int ret = SSL_do_handshake(ssl_);
        if (ret == 1)
        {
            state_ = SslState::ESTABLISHED;
            ZHTTP_LOG_INFO("SslConnection: handshake completed for peer {}", connection_->peerAddress().toIpPort());
            ZHTTP_LOG_INFO("SslConnection: cipher={}, version={}", SSL_get_cipher(ssl_), SSL_get_version(ssl_));
            if (!message_callback_)
            {
                ZHTTP_LOG_WARN("SslConnection: no message callback set after handshake for peer {}", connection_->peerAddress().toIpPort());
            }
            on_decrypted();
            if (message_callback_ && decrypted_buffer_.readableBytes() > 0)
                message_callback_(connection_, &decrypted_buffer_, receive_time_);
            return;
        }

        if (const int err = SSL_get_error(ssl_, ret); err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ)
        {
            ZHTTP_LOG_DEBUG("SslConnection: handshake needs more data (WANT_READ/WRITE) for peer {}", connection_->peerAddress().toIpPort());
            drain_write_bio();
            return;
        }

        const unsigned long ecode = ERR_get_error();
        char err_buf[256];
        ERR_error_string_n(ecode, err_buf, sizeof(err_buf));
        ZHTTP_LOG_ERROR("SslConnection: handshake failed for peer {}: {}", connection_->peerAddress().toIpPort(), err_buf);
        connection_->shutdown();
    }

    // 读取 BIO 中的加密数据，并发送
    void SslConnection::drain_write_bio()
    {
        char buf[4096];
        int total_sent = 0;
        while (int pend = BIO_pending(write_bio_))
        {
            if (const int n = BIO_read(write_bio_, buf, std::min(pend, static_cast<int>(sizeof(buf)))); n > 0)
            {
                write_buffer_.append(buf, n);
                connection_->send(&write_buffer_);
                total_sent += n;
            }
            else break;
        }
        if (total_sent > 0)
        {
            ZHTTP_LOG_DEBUG("SslConnection: sent {} bytes encrypted data to peer {}", total_sent, connection_->peerAddress().toIpPort());
        }
    }

    // 加密数据
    void SslConnection::on_encrypted(const void *data, size_t len) const
    {
        if (const int written = SSL_write(ssl_, data, static_cast<int>(len)); written <= 0)
        {
            const int err = SSL_get_error(ssl_, written);
            ZHTTP_LOG_ERROR("SslConnection: SSL_write failed for peer {}: {}", connection_->peerAddress().toIpPort(), ERR_error_string(err, nullptr));
            return;
        }
        ZHTTP_LOG_DEBUG("SslConnection: encrypted {} bytes for peer {}", len, connection_->peerAddress().toIpPort());
    }

    void SslConnection::on_decrypted()
    {
        char plain[4096];
        int total_decrypted = 0;
        while (true)
        {
            const int ret = SSL_read(ssl_, plain, sizeof(plain));
            if (ret > 0)
            {
                decrypted_buffer_.append(plain, ret);
                total_decrypted += ret;
                continue;
            }
            if (const int err = SSL_get_error(ssl_, ret); err == SSL_ERROR_WANT_READ)
            {
                break;
            }
            handle_error(get_last_error(ret));
            break;
        }
        if (total_decrypted > 0)
        {
            ZHTTP_LOG_DEBUG("SslConnection: decrypted {} bytes for peer {}", total_decrypted, connection_->peerAddress().toIpPort());
        }
    }

    SslError SslConnection::get_last_error(const int ret) const
    {
        switch (SSL_get_error(ssl_, ret))
        {
            case SSL_ERROR_NONE:
                return SslError::NONE;
            case SSL_ERROR_WANT_READ:
                return SslError::WANT_READ;
            case SSL_ERROR_WANT_WRITE:
                return SslError::WANT_WRITE;
            case SSL_ERROR_SYSCALL:
                return SslError::SYSCALL;
            case SSL_ERROR_SSL:
                return SslError::SSL;
            default:
                return SslError::UNKNOWN;
        }
    }

    void SslConnection::handle_error(SslError error)
    {
        const std::string peer_addr = connection_->peerAddress().toIpPort();
        switch (error)
        {
            case SslError::WANT_READ:
                ZHTTP_LOG_DEBUG("SslConnection: SSL needs more input data for peer {}", peer_addr);
                break;
            case SslError::WANT_WRITE:
                ZHTTP_LOG_DEBUG("SslConnection: SSL needs to write more data for peer {}", peer_addr);
                break;
            case SslError::SSL:
            case SslError::SYSCALL:
            case SslError::UNKNOWN:
                ZHTTP_LOG_ERROR("SslConnection: SSL error occurred for peer {}: {}", peer_addr, ERR_error_string(ERR_get_error(), nullptr));
                state_ = SslState::ERROR;
                connection_->shutdown();
                break;
            default:
                break;
        }
    }

    int SslConnection::bio_write(BIO *bio, const char *data, int len)
    {
        const auto *conn = static_cast<SslConnection *>(BIO_get_data(bio));
        if (!conn) return -1;

        conn->connection_->send(data, len);
        return len;
    }

    int SslConnection::bio_read(BIO *bio, char *data, int len)
    {
        auto *conn = static_cast<SslConnection *>(BIO_get_data(bio));
        if (!conn) return -1;

        const size_t readable = conn->read_buffer_.readableBytes();
        if (readable == 0)
        {
            return -1; // 无数据可读
        }

        const size_t to_read = std::min(static_cast<size_t>(len), readable);
        memcpy(data, conn->read_buffer_.peek(), to_read);
        conn->read_buffer_.retrieve(to_read);
        return static_cast<int>(to_read);
    }

    long SslConnection::bio_ctrl(BIO *bio, const int cmd, long num, void *ptr)
    {
        switch (cmd)
        {
            case BIO_CTRL_FLUSH:
                return 1;
            default:
                return 0;
        }
    }

    void SslConnection::set_message_callback(const MessageCallback &cb)
    {
        message_callback_ = cb;
        ZHTTP_LOG_DEBUG("SslConnection: message callback set for peer {}", connection_->peerAddress().toIpPort());
    }

    bool SslConnection::is_handshake_completed() const
    {
        return state_ == SslState::ESTABLISHED;
    }
} // namespace zhttp::zssl
