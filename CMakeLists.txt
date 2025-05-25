cmake_minimum_required(VERSION 3.15)
project(ZHttpServer)
set(CMAKE_CXX_STANDARD 17)

# 查找 Muduo 库
find_library(MUDUO_NET_LIB muduo_net REQUIRED)
find_library(MUDUO_BASE_LIB muduo_base REQUIRED)

# 查找 gtest
find_package(GTest REQUIRED)

# 检索所有头文件目录路径
file(GLOB_RECURSE ALL_HEADER_FILES ${PROJECT_SOURCE_DIR}/include/*.h)

# 创建一个包含所有头文件目录的列表
set(INCLUDE_DIRS "")
foreach(_headerFile ${ALL_HEADER_FILES})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND INCLUDE_DIRS ${_dir})
endforeach()

# 去重
list(REMOVE_DUPLICATES INCLUDE_DIRS)
include_directories(${INCLUDE_DIRS})

# 添加http相关的源文件
file(GLOB HTTP_SRC
        ${PROJECT_SOURCE_DIR}/source/http/*.cpp
)

# 添加路由相关的源文件
file(GLOB ROUTER_SRC
        ${PROJECT_SOURCE_DIR}/source/router/*.cpp
)

# 添加会话相关的源文件
file(GLOB SESSION_SRC
        ${PROJECT_SOURCE_DIR}/source/session/*.cpp
)

# 添加中间件相关的源文件
file(GLOB_RECURSE MIDDLEWARE_SRC
        ${PROJECT_SOURCE_DIR}/source/middleware/*.cpp
)

# 所有项目相关源文件
file(GLOB ALL_SRC
        ${HTTP_SRC}
        ${ROUTER_SRC}
        ${SESSION_SRC}
        ${MIDDLEWARE_SRC}
)


# 主程序可执行文件
add_executable(my_server main.cpp ${ALL_SRC})
target_link_libraries(my_server ${MUDUO_NET_LIB} ${MUDUO_BASE_LIB} pthread)

add_executable(unit_tests test/test.cpp ${ALL_SRC})
target_link_libraries(unit_tests GTest::GTest GTest::Main ${MUDUO_NET_LIB} ${MUDUO_BASE_LIB} pthread)

enable_testing()
add_test(NAME AllUnitTests COMMAND unit_tests)