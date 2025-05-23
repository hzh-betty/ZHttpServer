cmake_minimum_required(VERSION 3.15)
project(ZHttpServer)
set(CMAKE_CXX_STANDARD 17)

# 查找 Muduo 库
find_library(MUDUO_NET_LIB muduo_net)
find_library(MUDUO_BASE_LIB muduo_base)

# 查找 gtest
find_package(GTest REQUIRED)

# 主程序源文件
set(SERVER_SRC
        main.cpp
        include/http/http_context.h
        include/http/http_request.h
        source/http/http_context.cpp
        source/http/http_request.cpp
        include/http/http_response.h
        source/http/http_response.cpp
)

# 测试源文件
set(TEST_SRC
        test/http/test.cpp
        test/http/test_http_request.h
        test/http/test_http_context.h
        test/http/test_http_response.h
        source/http/http_request.cpp
        source/http/http_context.cpp
        source/http/http_response.cpp
)

# 主程序可执行文件
add_executable(my_server ${SERVER_SRC})
target_link_libraries(my_server ${MUDUO_NET_LIB} ${MUDUO_BASE_LIB} pthread)

# 测试可执行文件
add_executable(unit_tests ${TEST_SRC} )
target_link_libraries(unit_tests GTest::GTest GTest::Main ${MUDUO_NET_LIB} ${MUDUO_BASE_LIB} pthread)

# 启用测试
enable_testing()
add_test(NAME HttpRequestTest COMMAND unit_tests)