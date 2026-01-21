cmake_minimum_required(VERSION 3.10)

project(high_performance_server
    DESCRIPTION "A high performance web server that supports HTTP/1.1"
    LANGUAGES CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(src)

set(SRC_DIR src)
set(TEST_DIR test)

add_executable(high_performance_server
    ${SRC_DIR}/main.cc
    ${SRC_DIR}/http_server.cc
    ${SRC_DIR}/http_message.cc
    ${SRC_DIR}/socket.cc
)

add_executable(test_high_performance_server
    ${TEST_DIR}/main.cc
    ${SRC_DIR}/http_server.cc
    ${SRC_DIR}/http_message.cc
    ${SRC_DIR}/socket.cc
)

target_link_libraries(high_performance_server PRIVATE Threads::Threads)
target_link_libraries(test_high_performance_server PRIVATE Threads::Threads)
