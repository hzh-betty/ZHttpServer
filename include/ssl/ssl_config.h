#pragma once

#include "ssl_types.h"
#include <string>
#include <cstdint>

namespace zhttp::zssl
{
    class SslConfig
    {
    public:
        static SslConfig &get_instance()
        {
            static SslConfig instance;
            return instance;
        }

        ~SslConfig() = default;

        // 设置证书文件路径
        void set_cert_file_path(const std::string &cert_file_path);

        // 设置私钥文件路径
        void set_key_file_path(const std::string &key_file_path);

        // 设置证书链文件路径
        void set_chain_file_path(const std::string &chain_file_path);

        // 设置Ssl版本
        void set_version(SslVersion version);

        // 获取证书文件路径
        [[nodiscard]] std::string get_cert_file_path() const;
        [[nodiscard]] std::string get_key_file_path() const;
        [[nodiscard]] std::string get_chain_file_path() const;
        [[nodiscard]] SslVersion get_version() const;
        [[nodiscard]] std::string get_cipher_list() const;
        [[nodiscard]] bool get_verify_peer() const;
        [[nodiscard]] uint32_t get_verify_depth() const;
        [[nodiscard]] uint32_t get_session_timeout() const;
        [[nodiscard]] uint64_t get_session_cache_size() const;

    private:
        SslConfig();
    private:
        std::string cert_file_path_{}; // 证书文件路径
        std::string key_file_path_{}; // 私钥文件路径
        std::string chain_file_path_{}; // 证书链文件路径
        SslVersion version_;
        std::string cipher_list_{}; // 密码列表
        bool verify_peer_; // 是否验证对端
        uint32_t verify_depth_; // 验证深度
        uint32_t session_timeout_; // 会话超时时间
        uint64_t session_cache_size_; // 会话缓存大小
    };
} // namespace zhttp::zssl

