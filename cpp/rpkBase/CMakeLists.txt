cmake_minimum_required(VERSION 3.10)
project(rpkBase LANGUAGES C CXX)

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

option(WITH_RAYCASTING "Enable raycasting modules" ON)
option(WITH_MGF "Enable MGF reader modules" ON)
option(WITH_RAY_CAPTURE "Count visibility queries by path and write the ray corpus (step 1 capture build)" OFF)

if(UNIX AND NOT APPLE)
    add_compile_options(-std=c++11 -Wall -Wextra -pedantic)
endif()

if(APPLE)
    add_compile_options(-std=c++17 -Wall -pedantic)
endif()

add_compile_options(-O3)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE RPK_BASE_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/*.cpp
)

if(NOT WITH_MGF)
    list(FILTER RPK_BASE_SRC EXCLUDE REGEX "/io/mgf/")
endif()

if(NOT WITH_RAYCASTING)
    list(FILTER RPK_BASE_SRC EXCLUDE REGEX "/raycasting/")
endif()

add_library(rpkBase SHARED ${RPK_BASE_SRC})

target_include_directories(rpkBase
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main
)

target_link_libraries(rpkBase
    PUBLIC
        vitral
)

set_target_properties(rpkBase PROPERTIES
    OUTPUT_NAME rpkBase
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(WITH_RAYCASTING)
    target_compile_definitions(rpkBase PUBLIC RAYTRACING_ENABLED=true)
endif()

if(WITH_MGF)
    target_compile_definitions(rpkBase PUBLIC MGF_ENABLED=true)
endif()

if(WITH_RAY_CAPTURE)
    target_compile_definitions(rpkBase PUBLIC RPK_RAY_CAPTURE=true)
endif()
