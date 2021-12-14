cmake_minimum_required(VERSION 3.0.0)
project(threadpool VERSION 0.1.0)

add_library(taskqueue STATIC taskqueue.c)
add_library(threadpool STATIC threadpool.c)

target_link_libraries(threadpool taskqueue pthread)

add_executable(test test_threadpool.c)
target_link_libraries(test threadpool)
