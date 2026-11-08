cmake_minimum_required(VERSION 3.16)
project(HuffmanCompressionEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wconversion -Wno-sign-conversion -O3)
endif()

include_directories(include)

file(GLOB_RECURSE ENGINE_SOURCES "src/*.cpp")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "main\\.cpp$")

# Core Engine Library
add_library(huffman_engine STATIC ${ENGINE_SOURCES})

# Executable
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    add_executable(huffman src/main.cpp)
    target_link_libraries(huffman PRIVATE huffman_engine)
endif()

# Test Runner
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
if(TEST_SOURCES)
    add_executable(huffman_tests tests/test_all.cpp)
    target_link_libraries(huffman_tests PRIVATE huffman_engine)
endif()
