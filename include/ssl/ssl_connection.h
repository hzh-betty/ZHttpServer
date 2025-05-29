#pragma once

#include "ssl_context.h"
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>

namespace zhttp::zssl
{
    // 添加消息回调函数类型定义
    using MessageCallback = std::function<void(const std::shared_ptr<muduo::net::TcpConnection> &,
                                               muduo::net::Buffer *,
                                               muduo::Timestamp)>;

    class SslConnection
    {
    public:
        SslConnection(muduo::net::TcpConnectionPtr conn, SslContext *ctx);

        ~SslConnection();

        // SSL握手
        void handshake();

        // 发送数据
        void send(const void *data, size_t len);

        // 读取数据
        void on_read(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp time);

        // 判断握手是否完成
        [[nodiscard]] bool is_handshake_completed() const;

        // 获取解密缓冲区
        muduo::net::Buffer *get_decrypted_buffer() { return &decrypted_buffer_; }

        // SSL BIO 操作回调
        static int bio_write(BIO *bio, const char *data, int len);

        static int bio_read(BIO *bio, char *data, int len);

        static long bio_ctrl(BIO *bio, int cmd, long num, void *ptr);

        // 设置消息回调函数
        void set_message_callback(const MessageCallback &cb);

    private:
        // SSL握手处理
        void handle_handshake();

        // 加密解密处理
        void on_encrypted(const char *data, size_t len);

        void on_decrypted(const char *data, size_t len);

        // 获取错误信息
        SslError get_last_error(int ret);

        // 错误处理
        void handle_error(SslError error);

        // 读取write_bio数据
        void drain_write_bio();
    private:
        SSL *ssl_; // ssl连接
        SslContext *context_; // ssl上下文
        muduo::net::TcpConnectionPtr connection_;// tcp连接
        SslState state_; // ssl状态
        BIO *read_bio_; // 网络数据->ssl
        BIO *write_bio_; // ssl->网络数据
        muduo::net::Buffer read_buffer_; // 读取缓冲区
        muduo::net::Buffer write_buffer_; // 写入缓冲区
        muduo::net::Buffer decrypted_buffer_; //  解密缓冲区
        MessageCallback message_callback_; // 消息回调函数
    };
} // namespace zhttp::zssl