cmake_minimum_required(VERSION 3.9)
project(cctt)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(peglib INTERFACE)
target_include_directories(peglib INTERFACE "${CMAKE_CURRENT_LIST_DIR}/library/peglib/include")
target_link_libraries(peglib INTERFACE Threads::Threads)

file(GLOB_RECURSE cctt-sources source/*.cpp)
add_executable(cctt ${cctt-sources})
target_compile_features(cctt PUBLIC cxx_std_14)
target_link_libraries(cctt PRIVATE peglib)

