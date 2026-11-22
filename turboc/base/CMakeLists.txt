cmake_minimum_required(VERSION 3.10)
project(vitral-turboc LANGUAGES C CXX)

# The VITRAL library: base/src/main holds only the files listed in
# scripts/vitralSubset.txt, byte-identical to the VITRAL checkout
# (scripts/vitralSubset.sh --check). RenderPark's own classes are in rpkBase/.

# IEEE 754 floating point everywhere: no fast-math (no reassociation, no
# finite-math or reciprocal shortcuts) and no fused multiply-add contraction,
# so results do not depend on how the compiler rewrites an expression
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-fno-fast-math -ffp-contract=off)
endif()

option(FORCE_32BIT_UNIX_APPLE "Build 32-bit binaries on Linux/macOS (-m32)" ON)

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

file(GLOB_RECURSE VITRAL_TURBOC_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/*.cpp
)

# Keep the normal lowercase source tree only; uppercase 8.3 mirror files are not build inputs.
list(FILTER VITRAL_TURBOC_SRC EXCLUDE REGEX "/src/main/[A-Z0-9_]+/")

add_library(vitral-turboc SHARED ${VITRAL_TURBOC_SRC})

target_include_directories(vitral-turboc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main
)

set_target_properties(vitral-turboc PROPERTIES
    OUTPUT_NAME vitral-turboc
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
