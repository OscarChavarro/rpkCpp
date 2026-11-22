cmake_minimum_required(VERSION 3.10)
project(rpkBase-turboc LANGUAGES C CXX)

# RenderPark's own classes over VITRAL: the radiosity methods (galerkin,
# stochastic relaxation, random walk), the raytracers and raycasters, the scene
# model, MGF reading, tone mapping and their support. They keep the vsdk/...
# include paths they had inside base/; only the VITRAL copy stays in base/.

# IEEE 754 floating point everywhere: no fast-math (no reassociation, no
# finite-math or reciprocal shortcuts) and no fused multiply-add contraction,
# so results do not depend on how the compiler rewrites an expression
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-fno-fast-math -ffp-contract=off)
endif()

option(RAYTRACING_ENABLED "Enable raytracing modules" ON)
option(MGF_ENABLED "Enable MGF reader modules" ON)
option(FORCE_32BIT_UNIX_APPLE "Build 32-bit binaries on Linux/macOS (-m32)" ON)
option(WITH_RAY_CAPTURE "Count visibility queries by path and write the ray corpus (step 1 capture build)" OFF)

if(UNIX AND NOT APPLE)
    add_compile_options(-Wall -Wextra -pedantic)
endif()

if(APPLE)
    add_compile_options(-Wall -pedantic)
endif()

add_compile_options(-O3)

if((UNIX OR APPLE) AND FORCE_32BIT_UNIX_APPLE)
    # -mfpmath=sse keeps 32-bit float results consistent with the 64-bit cpp
    # reference build (the x87 default computes intermediates at 80 bits)
    add_compile_options(-m32 -msse2 -mfpmath=sse)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
endif()

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

file(GLOB_RECURSE RPK_BASE_TURBOC_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/*.cpp
)

# Keep the normal lowercase source tree only; uppercase 8.3 mirror files are not build inputs.
list(FILTER RPK_BASE_TURBOC_SRC EXCLUDE REGEX "/src/main/[A-Z0-9_]+/")

if(NOT MGF_ENABLED)
    list(FILTER RPK_BASE_TURBOC_SRC EXCLUDE REGEX "/io/mgf/")
    list(FILTER RPK_BASE_TURBOC_SRC EXCLUDE REGEX "/IO/MGF/")
endif()

if(NOT RAYTRACING_ENABLED)
    list(FILTER RPK_BASE_TURBOC_SRC EXCLUDE REGEX "/raycasting/")
    list(FILTER RPK_BASE_TURBOC_SRC EXCLUDE REGEX "/RAYCASTING/")
endif()

add_library(rpkBase-turboc SHARED ${RPK_BASE_TURBOC_SRC})

target_include_directories(rpkBase-turboc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main
)

target_link_libraries(rpkBase-turboc
    PUBLIC
        vitral-turboc
)

set_target_properties(rpkBase-turboc PROPERTIES
    OUTPUT_NAME rpkBase-turboc
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(RAYTRACING_ENABLED)
    target_compile_definitions(rpkBase-turboc PUBLIC RAYTRACING_ENABLED)
endif()

if(MGF_ENABLED)
    target_compile_definitions(rpkBase-turboc PUBLIC MGF_ENABLED)
endif()

# The ray corpus is written sequentially, with 64-bit file offsets so that the
# 32-bit build writes files beyond 2 GB
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/vsdk/toolkit/scene/VisibilityQueryRecorder.cpp
    PROPERTIES COMPILE_DEFINITIONS _FILE_OFFSET_BITS=64
)

if(WITH_RAY_CAPTURE)
    target_compile_definitions(rpkBase-turboc PUBLIC RPK_RAY_CAPTURE)
endif()
