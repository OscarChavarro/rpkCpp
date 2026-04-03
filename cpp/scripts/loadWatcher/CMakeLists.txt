cmake_minimum_required(VERSION 3.10)

project(LoadWatcher CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-O3)

add_executable(loadWatcher
    src/main.cpp
    src/LoadWatcher.cpp
    src/ProcReader.cpp
)
