cmake_minimum_required(VERSION 3.10)

project(DependencyGraphBuilder CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-O3)

add_executable(dependencyGraphBuilder
    src/main.cpp
)
