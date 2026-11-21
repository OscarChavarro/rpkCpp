cmake_minimum_required(VERSION 3.10)
project(nativeRandomNumberGenerator LANGUAGES CXX)

# IEEE 754 floating point everywhere: no fast-math (no reassociation, no
# finite-math or reciprocal shortcuts) and no fused multiply-add contraction,
# so results do not depend on how the compiler rewrites an expression
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-fno-fast-math -ffp-contract=off)
endif()

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
