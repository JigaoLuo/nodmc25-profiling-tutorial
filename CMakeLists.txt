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
    GIT_TAG "v0.11.1"
    PREFIX "lib/perf-cpp"
    INSTALL_COMMAND cmake -E echo ""
)

include(FetchContent)
FetchContent_Declare(
    perfevent
    GIT_REPOSITORY https://github.com/viktorleis/perfevent.git
    GIT_TAG master
)
FetchContent_MakeAvailable(perfevent)

FetchContent_Declare(
    nvtx
    GIT_REPOSITORY https://github.com/NVIDIA/NVTX
    GIT_TAG "v3.2.1"
)
FetchContent_MakeAvailable(nvtx)

#############################################################
# Execitable                                                #
#############################################################

include_directories(
    src
    lib/perf-cpp/src/perf-cpp-external/include
    ${perfevent_SOURCE_DIR}
    ${nvtx_SOURCE_DIR}
)   
link_directories(lib/perf-cpp/src/perf-cpp-external-build)

# Demo 1
add_executable(olc_coro_tree_perf
    src/main_perf.cpp
    src/workload/workload_set.cpp
    src/system.cpp
)
target_link_libraries(olc_coro_tree_perf pthread)
add_dependencies(olc_coro_tree_perf perf-cpp-external)

# Demo 2
add_executable(olc_coro_tree_perfevent
    src/main_perfevent.cpp
    src/workload/workload_set.cpp
    src/system.cpp
)
add_dependencies(olc_coro_tree_perfevent perf-cpp-external)
target_link_libraries(olc_coro_tree_perfevent pthread)

# Demo 3
add_executable(olc_coro_tree_perfcpp
    src/main_perfcpp.cpp
    src/workload/workload_set.cpp
    src/system.cpp
)
add_dependencies(olc_coro_tree_perfcpp perf-cpp-external)
target_link_libraries(olc_coro_tree_perfcpp perf-cpp pthread)

# Demo 4
add_executable(olc_coro_tree_nvtx
    src/main_nvtx.cpp
    src/workload/workload_set.cpp
    src/system.cpp
)
add_dependencies(olc_coro_tree_nvtx perf-cpp-external)
target_link_libraries(olc_coro_tree_nvtx perf-cpp pthread nvtx3-cpp)