cmake_minimum_required(VERSION 3.9)
project(cctt)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE cctt-sources source/*.cpp)
add_executable(cctt ${cctt-sources})
target_compile_features(cctt PUBLIC cxx_std_14)

