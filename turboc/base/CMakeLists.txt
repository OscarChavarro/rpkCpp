cmake_minimum_required(VERSION 3.10)
project(vitral-turboc LANGUAGES C CXX)

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

add_compile_options(-ffast-math -O3)

if((UNIX OR APPLE) AND FORCE_32BIT_UNIX_APPLE)
    # -mfpmath=sse keeps 32-bit float results consistent with the 64-bit cpp
    # reference build (the x87 default computes intermediates at 80 bits)
    add_compile_options(-m32 -msse2 -mfpmath=sse)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
endif()

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

file(GLOB_RECURSE VITRAL_TURBOC_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/*.cpp
)

# Keep the normal lowercase source tree only; uppercase 8.3 mirror files are not build inputs.
list(FILTER VITRAL_TURBOC_SRC EXCLUDE REGEX "/src/main/[A-Z0-9_]+/")

if(NOT MGF_ENABLED)
    list(FILTER VITRAL_TURBOC_SRC EXCLUDE REGEX "/io/mgf/")
    list(FILTER VITRAL_TURBOC_SRC EXCLUDE REGEX "/IO/MGF/")
endif()

if(NOT RAYTRACING_ENABLED)
    list(FILTER VITRAL_TURBOC_SRC EXCLUDE REGEX "/raycasting/")
    list(FILTER VITRAL_TURBOC_SRC EXCLUDE REGEX "/RAYCASTING/")
endif()

add_library(vitral-turboc SHARED ${VITRAL_TURBOC_SRC})

target_include_directories(vitral-turboc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main
)

set_target_properties(vitral-turboc PROPERTIES
    OUTPUT_NAME vitral-turboc
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(RAYTRACING_ENABLED)
    target_compile_definitions(vitral-turboc PUBLIC RAYTRACING_ENABLED)
endif()

if(MGF_ENABLED)
    target_compile_definitions(vitral-turboc PUBLIC MGF_ENABLED)
endif()

# The ray corpus is written sequentially, with 64-bit file offsets so that the
# 32-bit build writes files beyond 2 GB
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/vsdk/toolkit/scene/VisibilityQueryRecorder.cpp
    PROPERTIES COMPILE_DEFINITIONS _FILE_OFFSET_BITS=64
)

if(WITH_RAY_CAPTURE)
    target_compile_definitions(vitral-turboc PUBLIC RPK_RAY_CAPTURE)
endif()
