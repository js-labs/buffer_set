cmake_minimum_required(VERSION 3.10)

project(buffer_set)

option(BUILD_TESTS "Build tests" ON)
option(CODE_COVERAGE "Enable code coverage reporting" OFF)

include_directories(include)

set(LIB_SRCS
    include/buffer_set/buffer_set.h
    src/buffer_set.c
)

add_library(buffer_set STATIC ${LIB_SRCS})

if(BUILD_TESTS)
    if(CODE_COVERAGE AND CMAKE_C_COMPILER_ID MATCHES "GNU")
        message(NOTICE "** Building with code coverage flags")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    endif()

    set(TEST_SRCS
        tests/clear.c
        tests/insert.c
        tests/iterator_next.c
        tests/main.c
        tests/max_capacity.c
        tests/print_debug.c
        tests/random_op.c
        tests/reg.c
    )

    add_executable(buffer_set_tests ${TEST_SRCS})
    add_dependencies(buffer_set_tests buffer_set)
    target_link_libraries(buffer_set_tests buffer_set)

    add_executable(insert_perf tests/insert_perf.c)
    add_dependencies(insert_perf buffer_set)
    target_link_libraries(insert_perf buffer_set)

    include(CTest)
    add_test(NAME BufferSetTests COMMAND buffer_set_tests)
endif()
