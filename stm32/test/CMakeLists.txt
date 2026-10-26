cmake_minimum_required(VERSION 3.28)

project(test_mcutils_stm32 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/coverage.cmake)

set(SANITIZER_FLAGS -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
add_compile_options(${SANITIZER_FLAGS})
add_link_options(${SANITIZER_FLAGS})

# Fetch Unity unit testing framework
include(FetchContent)
FetchContent_Declare(unity
    GIT_REPOSITORY
    https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.7.0
)
set(FETCHCONTENT_UPDATES_DISCONNECTED_unity ON)
FetchContent_MakeAvailable(unity)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH SRC)

enable_testing()

# Adds a test executable from the given sources.
function(add_unit_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/stubs
        ${SRC}
    )
    target_compile_options(${name} PRIVATE
        -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Werror
    )
    target_link_libraries(${name} PRIVATE unity::framework)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_unit_test(test_fast_ring_buffer
    fast_ring_buffer.test.c
    ${SRC}/fast_ring_buffer.c
)

add_unit_test(test_stack_monitor
    stack_monitor.test.c
    ${SRC}/stack_monitor.c
)
# Non-PIE: PC-relative references to the absolute _Min_Stack_Size symbol aren't allowed in PIE.
target_compile_options(test_stack_monitor PRIVATE -fno-pie)
target_link_options(test_stack_monitor PRIVATE -no-pie)

coverage_add_target(INCLUDE ${SRC} EXCLUDE ${CMAKE_CURRENT_SOURCE_DIR})
