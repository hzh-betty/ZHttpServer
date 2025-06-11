#include "../../ssl/ssl_config.h"

#include <muduo/base/Logging.h>

namespace zhttp::zssl
{
    SslConfig::SslConfig()
    :version_(SslVersion::TLS_1_2)
    ,cipher_list_("HIGH:!aNULL:!MDS") 
    ,verify_peer_(false)
    ,verify_depth_(4)
    ,session_timeout_(300)
    ,session_cache_size_(20480L)
    {}

    // 设置证书文件路径
    void SslConfig::set_cert_file_path(const std::string &cert_file_path)
    {
        cert_file_path_ = cert_file_path;
    }

    // 设置私钥文件路径
    void SslConfig::set_key_file_path(const std::string &key_file_path)
    {
        key_file_path_ = key_file_path;
    }


    // 设置证书链文件路径
    void SslConfig::set_chain_file_path(const std::string &chain_file_path)
    {
        chain_file_path_ = chain_file_path;
    }

    // 设置版本
    void SslConfig::set_version(SslVersion version)
    {
        version_ = version;
    }
    // 获取证书文件路径
    std::string SslConfig::get_cert_file_path() const
    {
        return cert_file_path_;
    }
    std::string SslConfig::get_key_file_path() const
    {
        return key_file_path_;
    }
    std::string SslConfig::get_chain_file_path() const
    {
        return chain_file_path_;
    }

    SslVersion SslConfig::get_version() const
    {
        return version_;
    }

    std::string SslConfig::get_cipher_list() const
    {
        return cipher_list_;
    }
    bool SslConfig::get_verify_peer() const
    {
        return verify_peer_;
    }
    uint32_t SslConfig::get_verify_depth() const
    {
        return verify_depth_;
    }
    uint32_t SslConfig::get_session_timeout() const
    {
        return session_timeout_;
    }
    uint64_t SslConfig::get_session_cache_size() const
    {
        return session_cache_size_;
    }
}

