cmake_minimum_required(VERSION 3.10)
project(coro_tree_tutorial)

#############################################################
# CPP parameters                                            #
#############################################################

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -DNDEBUG")
set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

message("Building in '${CMAKE_BUILD_TYPE}' mode.")

#############################################################
# External Projects                                         #
#############################################################
include(ExternalProject)
ExternalProject_Add(
    perf-cpp-external
    GIT_REPOSITORY "https://github.com/jmuehlig/perf-cpp"
    GIT_TAG "v0.10.0"
    PREFIX "lib/perf-cpp"
    INSTALL_COMMAND cmake -E echo ""
)

#############################################################
# Execitable                                                #
#############################################################
include_directories(
    src
    lib/perf-cpp/src/perf-cpp-external/include
)
link_directories(lib/perf-cpp/src/perf-cpp-external-build)
add_executable(olc_coro_tree
    src/main.cpp
    src/workload/workload_set.cpp
    src/system.cpp
)

add_dependencies(olc_coro_tree perf-cpp-external)
target_link_libraries(olc_coro_tree perf-cpp numa)