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

# PNG output of VITRAL's ImagePersistence (the golden images are PNG files).
# The 32-bit build links the i386 libpng: Debian and Ubuntu multiarch systems
# may only have its runtime (libpng16-16:i386, without the libpng16.so link),
# and the headers of the native libpng-dev serve both architectures.
if((UNIX OR APPLE) AND FORCE_32BIT_UNIX_APPLE)
    find_path(RPK_PNG_INCLUDE_DIR png.h)
    find_library(RPK_PNG_LIBRARY
        NAMES png16 png libpng16.so.16
        PATHS /usr/lib/i386-linux-gnu /lib/i386-linux-gnu /usr/lib32 /usr/lib/i686-linux-gnu
        NO_DEFAULT_PATH)
    if(NOT RPK_PNG_INCLUDE_DIR OR NOT RPK_PNG_LIBRARY)
        message(FATAL_ERROR "32-bit libpng not found: install libpng-dev and libpng16-16:i386")
    endif()
    set(RPK_PNG_INCLUDE_DIRS ${RPK_PNG_INCLUDE_DIR})
    set(RPK_PNG_LIBRARIES ${RPK_PNG_LIBRARY})
else()
    find_package(PNG REQUIRED)
    set(RPK_PNG_INCLUDE_DIRS ${PNG_INCLUDE_DIRS})
    set(RPK_PNG_LIBRARIES ${PNG_LIBRARIES})
endif()
target_include_directories(vitral-turboc PRIVATE ${RPK_PNG_INCLUDE_DIRS})
target_link_libraries(vitral-turboc PRIVATE ${RPK_PNG_LIBRARIES})
target_compile_definitions(vitral-turboc PRIVATE VITRAL_WITH_PNG)

set_target_properties(vitral-turboc PROPERTIES
    OUTPUT_NAME vitral-turboc
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
