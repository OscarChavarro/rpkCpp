cmake_minimum_required(VERSION 3.10)
project(vitral LANGUAGES C CXX)

# The VITRAL library: base/src/main holds only the files listed in
# scripts/vitralSubset.txt, byte-identical to the VITRAL checkout
# (scripts/vitralSubset.sh --check). RenderPark's own classes are in rpkBase/.

# IEEE 754 floating point everywhere: no fast-math (no reassociation, no
# finite-math or reciprocal shortcuts) and no fused multiply-add contraction,
# so results do not depend on how the compiler rewrites an expression
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-fno-fast-math -ffp-contract=off)
endif()

if(UNIX AND NOT APPLE)
    add_compile_options(-std=c++11 -Wall -Wextra -pedantic)
endif()

if(APPLE)
    add_compile_options(-std=c++17 -Wall -pedantic)
endif()

add_compile_options(-O3)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE VITRAL_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/vsdk/toolkit/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/java/*.cpp
)

# Without semantic interposition, PlanarFace's ray kernel (rayHit and the
# functions it calls) is inlined inside the file instead of called through
# the PLT of the shared library; this changes no arithmetic
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/vsdk/toolkit/environment/geometry/element/PlanarFace.cpp
    PROPERTIES COMPILE_OPTIONS "-fno-semantic-interposition")

add_library(vitral SHARED ${VITRAL_SRC})

# java::Thread runs over POSIX threads (parallel Jacobi, plan step 7)
find_package(Threads REQUIRED)
target_link_libraries(vitral PUBLIC Threads::Threads)

# PNG export of the rendered images (ImagePersistence::exportPNG), the format
# of the golden images
find_package(PNG REQUIRED)
target_include_directories(vitral PRIVATE ${PNG_INCLUDE_DIRS})
target_link_libraries(vitral PRIVATE ${PNG_LIBRARIES})
target_compile_definitions(vitral PRIVATE VITRAL_WITH_PNG)

target_include_directories(vitral
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main
)

set_target_properties(vitral PROPERTIES
    OUTPUT_NAME vitral
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
