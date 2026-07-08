cmake_minimum_required(VERSION 3.10)
project(nativeRandomNumberGenerator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(nativeRandomNumberGenerator SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/NativeRandom.cpp
)

target_include_directories(nativeRandomNumberGenerator
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(nativeRandomNumberGenerator PROPERTIES
    OUTPUT_NAME rpkNativeRandom
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
