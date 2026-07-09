cmake_minimum_required(VERSION 3.10)
project(vitral-turboc LANGUAGES C CXX)

option(RAYTRACING_ENABLED "Enable raytracing modules" ON)
option(MGF_ENABLED "Enable MGF reader modules" ON)
option(FORCE_32BIT_UNIX_APPLE "Build 32-bit binaries on Linux/macOS (-m32)" ON)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Keep the normal lowercase source tree only; uppercase 8.3 mirror files are not build inputs.
list(FILTER VITRAL_TURBOC_SRC EXCLUDE REGEX "/src/[A-Z0-9_]+/")

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
        ${CMAKE_CURRENT_SOURCE_DIR}/src
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
