cmake_minimum_required(VERSION 4.0)

project(buffer_set)

option(BUILD_TESTS "Build tests" ON)

include_directories(include)

set(LIB_SRCS
    include/buffer_set/buffer_set.h
    src/buffer_set.c
)

add_library(buffer_set STATIC ${LIB_SRCS})

if(BUILD_TESTS)
    set(TEST_SRCS
        tests/insert.c
        tests/main.c
        tests/max_capacity.c
        tests/random_op.c
        tests/reg.c
        tests/walk.c
    )

    add_executable(buffer_set_tests ${TEST_SRCS})
    add_dependencies(buffer_set_tests buffer_set)
    target_link_libraries(buffer_set_tests buffer_set)

    add_executable(insert_perf tests/insert_perf.c)
    add_dependencies(insert_perf buffer_set)
    target_link_libraries(insert_perf buffer_set)
endif()

