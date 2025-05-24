cmake_minimum_required(VERSION 3.15)
project(ZHttpServer)
set(CMAKE_CXX_STANDARD 17)

# 查找 Muduo 库
find_library(MUDUO_NET_LIB muduo_net REQUIRED)
find_library(MUDUO_BASE_LIB muduo_base REQUIRED)

# 查找 gtest
find_package(GTest REQUIRED)

# 头文件目录
include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/test
)

# 添加http相关的源文件
file(GLOB HTTP_SRC
        ${PROJECT_SOURCE_DIR}/source/http/*.cpp
)

# 添加路由相关的源文件
file(GLOB ROUTER_SRC
        ${PROJECT_SOURCE_DIR}/source/router/*.cpp
)

# 添加测试相关的源文件
file(GLOB TEST_SRC
        ${PROJECT_SOURCE_DIR}/test/*.cpp
)


# 所有项目相关源文件
file(GLOB ALL_SRC
        ${HTTP_SRC}
        ${ROUTER_SRC}
)


# 主程序可执行文件
add_executable(my_server main.cpp ${ALL_SRC})
target_link_libraries(my_server ${MUDUO_NET_LIB} ${MUDUO_BASE_LIB} pthread)

add_executable(unit_tests ${TEST_SRC} ${ALL_SRC})
target_link_libraries(unit_tests GTest::GTest GTest::Main ${MUDUO_NET_LIB} ${MUDUO_BASE_LIB} pthread)

enable_testing()
add_test(NAME AllUnitTests COMMAND unit_tests)