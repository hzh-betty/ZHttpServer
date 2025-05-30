#pragma once

namespace zhttp::zssl
{
    // Ssl协议版本
    enum class SslVersion
    {
        TLS_1_0,
        TLS_1_1,
        TLS_1_2,
        TLS_1_3
    };

    // Ssl错误类型
    enum class SslError
    {
        NONE,
        WANT_READ,
        WANT_WRITE,
        SYSCALL,
        SSL,
        UNKNOWN
    };

    // Ssl 状态
    enum class SslState
    {
        HANDSHAKE,
        ESTABLISHED,
        ERROR
    };
} // namespace zhttp::zssl