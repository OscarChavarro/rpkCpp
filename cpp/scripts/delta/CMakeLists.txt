cmake_minimum_required(VERSION 3.10)

project(ImageDiff CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-ffast-math -O3)

add_executable(imageDiff
    src/imageDiff.cpp
    src/PixelRGB.cpp
    src/ImagePPM.cpp
    src/PPMReader.cpp
    src/PPMWriter.cpp
    src/ImageComparator.cpp
    src/Stats.cpp
)
