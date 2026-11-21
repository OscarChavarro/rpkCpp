cmake_minimum_required(VERSION 3.10)

project(ImageDiff CXX)

# IEEE 754 floating point everywhere: no fast-math (no reassociation, no
# finite-math or reciprocal shortcuts) and no fused multiply-add contraction,
# so results do not depend on how the compiler rewrites an expression
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-fno-fast-math -ffp-contract=off)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-O3)

add_executable(imageDiff
    src/imageDiff.cpp
    src/PixelRGB.cpp
    src/ImagePPM.cpp
    src/PPMReader.cpp
    src/PPMWriter.cpp
    src/ImageComparator.cpp
    src/Stats.cpp
)
