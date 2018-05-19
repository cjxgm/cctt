cmake_minimum_required(VERSION 3.9)
project(cctt)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(nonstd INTERFACE)
target_include_directories(nonstd INTERFACE ${CMAKE_CURRENT_LIST_DIR}/library/nonstd/include)

add_library(fmt INTERFACE)
target_include_directories(fmt INTERFACE ${CMAKE_CURRENT_LIST_DIR}/library/fmt/include)

file(GLOB_RECURSE cctt-sources source/*.cpp)
add_executable(cctt ${cctt-sources})
target_compile_features(cctt PUBLIC cxx_std_14)
target_link_libraries(
    cctt PUBLIC
    nonstd
    fmt
)

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    message("Using ccache: ${CCACHE_FOUND}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif ()

