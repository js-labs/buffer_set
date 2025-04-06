cmake_minimum_required(VERSION 2.8)

project(buffer_set)

option(BUILD_TESTS "Build tests" ON)

include_directories(include)

set(LIB_SRCS
    include/buffer_set/buffer_set.h
    src/buffer_set.c
)

add_library(buffer_set SHARED ${LIB_SRCS})
target_compile_definitions(buffer_set PRIVATE BUILDING_BUFFER_SET_DLL)

if(BUILD_TESTS)
    set(TEST_SRCS tests/main.c)

    add_executable(buffer_set_tests ${TEST_SRCS})
    add_dependencies(buffer_set_tests buffer_set)

    target_link_libraries(buffer_set_tests buffer_set)
endif()
