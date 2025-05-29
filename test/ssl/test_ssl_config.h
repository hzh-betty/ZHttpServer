
#pragma once

#include <gtest/gtest.h>
#include "../../include/ssl/ssl_config.h"

namespace zhttp::zssl
{

    TEST(SslConfigTest, SingletonPattern)
    {
        // 验证单例模式唯一性
        SslConfig &instance1 = SslConfig::get_instance();
        SslConfig &instance2 = SslConfig::get_instance();
        ASSERT_EQ(&instance1, &instance2);
    }

    TEST(SslConfigTest, DefaultConfiguration)
    {
        SslConfig &config = SslConfig::get_instance();

        // 验证默认SSL版本
        ASSERT_EQ(config.get_version(), SslVersion::TLS_1_2);

        // 验证默认密码套件
        ASSERT_EQ(config.get_cipher_list(), "HIGH:!aNULL:!MDS");

        // 验证默认禁用对端验证
        ASSERT_FALSE(config.get_verify_peer());

        // 验证默认会话参数
        ASSERT_EQ(config.get_verify_depth(), 4);
        ASSERT_EQ(config.get_session_timeout(), 300);
        ASSERT_EQ(config.get_session_cache_size(), 20480L);
    }

    TEST(SslConfigTest, PathConfiguration)
    {
        SslConfig &config = SslConfig::get_instance();

        // 测试证书路径设置
        const std::string certPath = "/path/to/cert.pem";
        config.set_cert_file_path(certPath);
        ASSERT_EQ(config.get_cert_file_path(), certPath);

        // 测试私钥路径设置
        const std::string keyPath = "/path/to/key.pem";
        config.set_key_file_path(keyPath);
        ASSERT_EQ(config.get_key_file_path(), keyPath);

        // 测试证书链路径设置
        const std::string chainPath = "/path/to/chain.pem";
        config.set_chain_file_path(chainPath);
        ASSERT_EQ(config.get_chain_file_path(), chainPath);
    }

} // namespace zhttp::zssl
