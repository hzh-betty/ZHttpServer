cmake_minimum_required(VERSION 3.15)
project(ZHttpServer)
set(CMAKE_CXX_STANDARD 17)

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

# 检索所有头文件目录路径
file(GLOB_RECURSE ALL_HEADER_FILES
        ${PROJECT_SOURCE_DIR}/zlog/*.hpp
        ${PROJECT_SOURCE_DIR}/include/*.h
        ${PROJECT_SOURCE_DIR}/test/*.h)

set(INCLUDE_DIRS "")
foreach (_headerFile ${ALL_HEADER_FILES})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND INCLUDE_DIRS ${_dir})
endforeach ()
list(REMOVE_DUPLICATES INCLUDE_DIRS)
include_directories(${INCLUDE_DIRS})

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

set_target_properties(zhttpserver PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/lib/debug
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/lib/release
)

# 链接其他第三方库
target_link_libraries(zhttpserver
        ${MUDUO_NET_LIB}
        ${MUDUO_BASE_LIB}
        ${MYSQL_LIB}
        ${MYSQLCPPCONN_LIB}
        ${SSL_LIB}
        ${CRYPTO_LIB}
        pthread
        fmt::fmt
)

# 主程序和单元测试只链接动态库
add_executable(server example/example.cpp)
target_link_libraries(server zhttpserver)

add_executable(unit_tests test/test.cpp)
target_link_libraries(unit_tests
        zhttpserver
        GTest::GTest
        GTest::Main
)

# 单元测试
enable_testing()
add_test(NAME AllUnitTests COMMAND unit_tests)