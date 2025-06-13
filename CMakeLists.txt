cmake_minimum_required(VERSION 3.15)
project(ZHttpServer)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finput-charset=UTF-8 -fexec-charset=UTF-8")

#查找fmt库
find_package(fmt REQUIRED)

# 查找 Muduo 库
find_library(MUDUO_NET_LIB muduo_net REQUIRED)
find_library(MUDUO_BASE_LIB muduo_base REQUIRED)

# 查找 gtest
find_package(GTest REQUIRED)

# MySQL C 客户端
find_library(MYSQL_LIB
        NAMES mysqlclient mysqlclient_r
        PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib
        REQUIRED
)

# Connector/C++
find_library(MYSQLCPPCONN_LIB
        NAMES mysqlcppconn mysqlcppconn8
        PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib
        REQUIRED
)

# 查找 OpenSSL 的 ssl 和 crypto
find_library(SSL_LIB ssl REQUIRED)
find_library(CRYPTO_LIB crypto REQUIRED)

# OpenSSL 查找
find_package(OpenSSL REQUIRED)

# 查找 redis-plus-plus
find_library(REDIS_PLUS_PLUS_LIB
    NAMES redis++ redis-plus-plus
    PATHS /usr/local/lib /usr/lib /usr/lib64
    REQUIRED
)

# 查找 hiredis
find_library(HIREDIS_LIB
    NAMES hiredis
    PATHS /usr/local/lib /usr/lib /usr/lib64
    REQUIRED
)

# 源文件收集
file(GLOB DB_POOL_SRC ${PROJECT_SOURCE_DIR}/source/db_pool/*.cpp)
file(GLOB HTTP_SRC ${PROJECT_SOURCE_DIR}/source/http/*.cpp)
file(GLOB ROUTER_SRC ${PROJECT_SOURCE_DIR}/source/router/*.cpp)
file(GLOB SESSION_SRC ${PROJECT_SOURCE_DIR}/source/session/*.cpp)
file(GLOB_RECURSE MIDDLEWARE_SRC ${PROJECT_SOURCE_DIR}/source/middleware/*.cpp)
file(GLOB_RECURSE SSL_SRC ${PROJECT_SOURCE_DIR}/source/ssl/*.cpp)

file(GLOB ALL_SRC
        ${HTTP_SRC}
        ${ROUTER_SRC}
        ${SESSION_SRC}
        ${MIDDLEWARE_SRC}
        ${DB_POOL_SRC}
        ${SSL_SRC}
)

# 生成动态库
add_library(zhttpserver SHARED ${ALL_SRC})

# 设置目标包含目录 - 使用 target_include_directories 替代全局 include_directories
target_include_directories(zhttpserver
    PUBLIC
        # 公共头文件目录，供外部项目使用
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/zlog>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(zhttpserver PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/lib/debug
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/lib/release
)

# 链接其他第三方库
target_link_libraries(zhttpserver
        PUBLIC
            ${MUDUO_NET_LIB}
            ${MUDUO_BASE_LIB}
            fmt::fmt
            ${MYSQL_LIB}
            ${MYSQLCPPCONN_LIB}
            ${SSL_LIB}
            ${CRYPTO_LIB}
            ${REDIS_PLUS_PLUS_LIB}
            ${HIREDIS_LIB}
            pthread
)

# 主程序
add_executable(server example/example.cpp)
target_link_libraries(server PRIVATE zhttpserver)

# 单元测试
add_executable(unit_tests test/test.cpp)
target_include_directories(unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/test)
target_link_libraries(unit_tests
        PRIVATE
            zhttpserver
            GTest::GTest
            GTest::Main
)

# 单元测试
enable_testing()
add_test(NAME AllUnitTests COMMAND unit_tests)

# 安装规则（可选）
install(TARGETS zhttpserver
    EXPORT ZHttpServerTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY zlog/ DESTINATION include/zlog
    FILES_MATCHING PATTERN "*.hpp"
)