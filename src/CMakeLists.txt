cmake_minimum_required(VERSION 3.10)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")

# set the project name
project(th_png2mks VERSION 0.2.1)

# add the executable
add_executable(th_png2mks main.cpp picopng.cpp)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

target_compile_features(th_png2mks PRIVATE cxx_std_17)
