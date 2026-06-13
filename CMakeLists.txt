cmake_minimum_required(VERSION 3.16)
project(flowq)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED hiredis)

add_executable(flowq_worker
    src/queue/redis_queue.cpp
    src/worker/worker.cpp
)

target_include_directories(flowq_worker PRIVATE ${HIREDIS_INCLUDE_DIRS})
target_link_libraries(flowq_worker ${HIREDIS_LIBRARIES})