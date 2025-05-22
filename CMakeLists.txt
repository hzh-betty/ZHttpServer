cmake_minimum_required(VERSION 3.15)
project(ZHttpServer)
set(CMAKE_CXX_STANDARD 17)

find_library(MUDUO_NET_LIB muduo_net)
find_library(MUDUO_BASE_LIB muduo_base)

add_executable(my_server main.cpp
        include/http_context.h
        include/http_request.h
        source/http_context.cpp
        source/http_request.cpp)

# 链接Muduo库和pthread
target_link_libraries(my_server ${MUDUO_NET_LIB} ${MUDUO_BASE_LIB} pthread)