cmake_minimum_required(VERSION 3.16)
project(flowq)
set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED hiredis)
pkg_check_modules(LIBPQ REQUIRED libpq)

add_executable(flowq_pool
    src/queue/redis_queue.cpp
    src/worker/worker.cpp
    src/worker/pool.cpp
    src/worker/heartbeat.cpp
    src/db/postgres.cpp
)
target_include_directories(flowq_pool PRIVATE
    ${HIREDIS_INCLUDE_DIRS}
    ${LIBPQ_INCLUDE_DIRS}
)
target_link_libraries(flowq_pool
    ${HIREDIS_LIBRARIES}
    ${LIBPQ_LIBRARIES}
)

add_executable(flowq_api
    src/api/Server.cpp
    src/common/Priority.cpp
    src/queue/redis_queue.cpp
    src/db/postgres.cpp
)
target_include_directories(flowq_api PRIVATE
    ${HIREDIS_INCLUDE_DIRS}
    ${LIBPQ_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/third_party/crow
)
target_link_libraries(flowq_api
    ${HIREDIS_LIBRARIES}
    ${LIBPQ_LIBRARIES}
    pthread
)

add_executable(flowq_Scheduler
        src/Scheduler/Scheduler.cpp
        src/queue/redis_queue.cpp
)
target_include_directories(flowq_Scheduler PRIVATE
    ${HIREDIS_INCLUDE_DIRS}
    
)
target_link_libraries(flowq_Scheduler
    ${HIREDIS_LIBRARIES}
    
)