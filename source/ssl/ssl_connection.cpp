#include "ssl/ssl_connection.h"
#include <muduo/base/Logging.h>
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
        // 创建 SSL 对象
        ssl_ = SSL_new(context_->get_context());
        if (!ssl_)
        {
            LOG_ERROR << "Failed to create SSL object: " << ERR_error_string(ERR_get_error(), nullptr);
            return;
        }

        // 创建 BIO
        read_bio_ = BIO_new(BIO_s_mem());
        write_bio_ = BIO_new(BIO_s_mem());

        if (!read_bio_ || !write_bio_)
        {
            LOG_ERROR << "Failed to create BIO objects";
            SSL_free(ssl_);
            ssl_ = nullptr;
            return;
        }

        // 设置 BIO
        SSL_set_bio(ssl_, read_bio_, write_bio_);
        SSL_set_accept_state(ssl_);  // 设置为服务器模式

        // 设置非阻塞 IO
        SSL_set_mode(ssl_, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
        SSL_set_mode(ssl_, SSL_MODE_ENABLE_PARTIAL_WRITE);

        // 收到数据时，调用此回调
        connection_->setMessageCallback(
                [this](auto &&PH1, auto &&PH2, auto &&PH3)
                {
                    on_read(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
                            std::forward<decltype(PH3)>(PH3));
                });

        LOG_INFO << "SSL connection created";
    }

    SslConnection::~SslConnection()
    {
        if (ssl_)
        {
            SSL_free(ssl_);  // 这会同时释放 BIO
        }
    }

    // 处理 SSL 握手
    void SslConnection::handshake()
    {
        SSL_set_accept_state(ssl_); // 设置为服务器模式
        handle_handshake();
    }

    // BIO 方法
    void SslConnection::send(const void *data, size_t len)
    {
        if (state_ != SslState::ESTABLISHED)
        {
            LOG_ERROR << "Cannot send data before SSL handshake is complete";
            return;
        }

        int written = SSL_write(ssl_, data, (int) len);
        if (written <= 0)
        {
            int err = SSL_get_error(ssl_, written);
            LOG_ERROR << "SSL_write failed: " << ERR_error_string(err, nullptr);
            return;
        }

        // 把 write_bio_ 里的“加密后数据”取出来，通过 TCP 发给客户
        char buf[4096];
        int pending;
        while ((pending = BIO_pending(write_bio_)) > 0)
        {
            int bytes = BIO_read(write_bio_, buf,
                                 std::min(pending, static_cast<int>(sizeof(buf))));
            if (bytes > 0)
            {
                connection_->send(buf, bytes);
            }
        }
    }

    void SslConnection::on_read(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf,
                                muduo::Timestamp time)
    {
//        if (state_ == SslState::HANDSHAKE)
//        {
//            LOG_INFO << "Received data from client ";
//
//            // 将数据写入 BIO
//            BIO_write(read_bio_, buf->peek(), (int) buf->readableBytes());
//            buf->retrieve(buf->readableBytes());
//            handle_handshake();
//            return;
//        }
//        else if (state_ == SslState::ESTABLISHED)
//        {
//            // 解密数据
//            char decrypted_data[4096];
//            LOG_INFO << "int ret = SSL_read(ssl_, decrypted_data, sizeof(decrypted_data)); ";
//            int ret = SSL_read(ssl_, decrypted_data, sizeof(decrypted_data));
//            if (ret > 0)
//            {
//                // 创建新的 Buffer 存储解密后的数据
//                muduo::net::Buffer decrypted_buffer;
//                decrypted_buffer.append(decrypted_data, ret);
//
//                // 调用上层回调处理解密后的数据
//                if (message_callback_)
//                {
//                    message_callback_(conn, &decrypted_buffer, time);
//                }
//                LOG_INFO << "on_read has been called";
//            }
//        }
        // 1. 写入所有接收到的加密数据到读 BIO
        LOG_INFO << "Received data from client ";
        int n = buf->readableBytes();
        if (n > 0)
        {
            BIO_write(read_bio_, buf->peek(), n);
            buf->retrieve(n);
        }

        // 2. 握手阶段
        if (state_ == SslState::HANDSHAKE)
        {
            handle_handshake();
            return;
        }

        if(state_ != SslState::ESTABLISHED)
        {
            LOG_ERROR << "SSL handshake failed";
            return;
        }
        // 3. 应用数据阶段：循环读取明文
        char plain[4096];
        while (true)
        {
            int ret = SSL_read(ssl_, plain, sizeof(plain));
            if (ret > 0)
            {
                muduo::net::Buffer decrypted;
                decrypted.append(plain, ret);
                if (message_callback_)
                {
                    message_callback_(conn, &decrypted, time);
                }
                continue;
            }
            int err = SSL_get_error(ssl_, ret);
            if (err == SSL_ERROR_WANT_READ)
            {
                // 明文读完，等待新的密文
                break;
            }
            // 其他错误
            handle_error(get_last_error(ret));
            break;
        }
    }

    void SslConnection::handle_handshake()
    {
//        int ret = SSL_do_handshake(ssl_);
//
//        if (ret == 1)
//        {
//            state_ = SslState::ESTABLISHED;
//            LOG_INFO << "SSL handshake completed successfully";
//            LOG_INFO << "Using cipher: " << SSL_get_cipher(ssl_);
//            LOG_INFO << "Protocol version: " << SSL_get_version(ssl_);
//
//            // 握手完成后，确保设置正确回调
//            if (!message_callback_)
//            {
//                LOG_WARN << "No message callback set after SSL handshake";
//            }
//
//            return;
//        }
//
//        int err = SSL_get_error(ssl_, ret);
//        switch (err)
//        {
//            case SSL_ERROR_WANT_READ:
//            case SSL_ERROR_WANT_WRITE:
//                // 正常握手过程，需要继续
//                drain_write_bio();
//                break;
//
//            default:
//            {
//                // 获取详细的错误信息
//                char err_buf[256];
//                unsigned long err_code = ERR_get_error();
//                ERR_error_string_n(err_code, err_buf, sizeof(err_buf));
//                LOG_ERROR << "SSL handshake failed: " << err_buf;
//                connection_->shutdown();  // 关闭连接
//                break;
//            }
//        }
//        LOG_INFO << "SSL handshake in progress " << err;
        int ret = SSL_do_handshake(ssl_);
        if (ret == 1)
        {
            state_ = SslState::ESTABLISHED;
            LOG_INFO << "SSL handshake completed successfully";
            LOG_INFO << "Using cipher: " << SSL_get_cipher(ssl_);
            LOG_INFO << "Protocol version: " << SSL_get_version(ssl_);

            // 握手完成后，确保设置了正确的回调
            if (!message_callback_) {
                LOG_WARN << "No message callback set after SSL handshake";
            }
            muduo::Timestamp time;
            on_read(connection_, &decrypted_buffer_, time);
            return;
        }

        int err = SSL_get_error(ssl_, ret);
        LOG_INFO << "SSL handshake in progress " << err;
        if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ)
        {
            // 还有握手数据要发
            drain_write_bio();
            return;
        }

        // 握手失败
        unsigned long ecode = ERR_get_error();
        char err_buf[256];
        ERR_error_string_n(ecode, err_buf, sizeof(err_buf));
        LOG_ERROR << "SSL handshake failed: " << err_buf;
        connection_->shutdown();
    }

    void SslConnection::drain_write_bio()
    {
        char buf[4096];
        while (int pend = BIO_pending(write_bio_))
        {
            int n = BIO_read(write_bio_, buf, std::min(pend, (int) sizeof(buf)));
            if (n > 0) connection_->send(buf, n);
            else break;
        }
    }

    void SslConnection::on_encrypted(const char *data, size_t len)
    {
        write_buffer_.append(data, len);
        connection_->send(&write_buffer_);
    }

    void SslConnection::on_decrypted(const char *data, size_t len)
    {
        decrypted_buffer_.append(data, len);
    }

    SslError SslConnection::get_last_error(int ret)
    {
        int err = SSL_get_error(ssl_, ret);
        switch (err)
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
        switch (error)
        {
            case SslError::WANT_READ:
            case SslError::WANT_WRITE:
                // 需要等待更多数据或写入缓冲区可用
                break;
            case SslError::SSL:
            case SslError::SYSCALL:
            case SslError::UNKNOWN:
                LOG_ERROR << "SSL error occurred: " << ERR_error_string(ERR_get_error(), nullptr);
                state_ = SslState::ERROR;
                connection_->shutdown();
                break;
            default:
                break;
        }
    }

    int SslConnection::bio_write(BIO *bio, const char *data, int len)
    {
        auto *conn = static_cast<SslConnection *>(BIO_get_data(bio));
        if (!conn) return -1;

        conn->connection_->send(data, len);
        return len;
    }

    int SslConnection::bio_read(BIO *bio, char *data, int len)
    {
        auto *conn = static_cast<SslConnection *>(BIO_get_data(bio));
        if (!conn) return -1;

        size_t readable = conn->read_buffer_.readableBytes();
        if (readable == 0)
        {
            return -1;  // 无数据可读
        }

        size_t to_read = std::min(static_cast<size_t>(len), readable);
        memcpy(data, conn->read_buffer_.peek(), to_read);
        conn->read_buffer_.retrieve(to_read);
        return to_read;
    }

    long SslConnection::bio_ctrl(BIO *bio, int cmd, long num, void *ptr)
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
    }

    bool SslConnection::is_handshake_completed() const
    {
        return state_ == SslState::ESTABLISHED;
    }
} // namespace zhttp::zssl