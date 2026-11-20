cmake_minimum_required(VERSION 3.10)
project(vitral LANGUAGES C CXX)

option(WITH_RAYCASTING "Enable raycasting modules" ON)
option(WITH_MGF "Enable MGF reader modules" ON)
option(WITH_RAY_CAPTURE "Count visibility queries by path and write the ray corpus (step 1 capture build)" OFF)

if(UNIX AND NOT APPLE)
    add_compile_options(-std=c++11 -Wall -Wextra -pedantic)
endif()

if(APPLE)
    add_compile_options(-std=c++17 -Wall -pedantic)
endif()

add_compile_options(-ffast-math -O3)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE VITRAL_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/vsdk/toolkit/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/java/*.cpp
)

if(NOT WITH_MGF)
    list(FILTER VITRAL_SRC EXCLUDE REGEX "/io/mgf/")
endif()

if(NOT WITH_RAYCASTING)
    list(FILTER VITRAL_SRC EXCLUDE REGEX "/raycasting/")
endif()

# PlanarFace reproduces RenderPark's face kernels in source order: under
# fast-math, the unrolled vertex loops are reassociated and the results move.
# Without semantic interposition, the ray kernel (rayHit and the functions it
# calls) is inlined inside the file instead of called through the PLT of the
# shared library; this changes no arithmetic
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/vsdk/toolkit/environment/geometry/element/PlanarFace.cpp
    PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-fno-semantic-interposition")

add_library(vitral SHARED ${VITRAL_SRC})

target_include_directories(vitral
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main
)

set_target_properties(vitral PROPERTIES
    OUTPUT_NAME vitral
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(WITH_RAYCASTING)
    target_compile_definitions(vitral PUBLIC RAYTRACING_ENABLED=true)
endif()

if(WITH_MGF)
    target_compile_definitions(vitral PUBLIC MGF_ENABLED=true)
endif()

if(WITH_RAY_CAPTURE)
    target_compile_definitions(vitral PUBLIC RPK_RAY_CAPTURE=true)
endif()
