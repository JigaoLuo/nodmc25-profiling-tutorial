#!/bin/bash

print_message() {
  local message="$*"
  local len=${#message}

  local border
  border=$(printf '%*s' $((len + 6)) "" | tr ' ' '#')

  echo "$border"
  echo "# $message   #"
  echo "$border"
}

## Build olc_tree
print_message "Building coro-tree"
cmake . -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j4

# DEMO 1 Binary
echo ""
print_message "DEMO 1 Binary: Executing coro-tree: perf"
print_message "ADD: perf stat, perf record, perf report ..."
./bin/olc_coro_tree_perf

# DEMO 2 Binary
echo ""
print_message "DEMO 2: Executing coro-tree: perf-event"
./bin/olc_coro_tree_perfevent

# DEMO 3 Binary
echo ""
print_message "DEMO 3: Executing coro-tree: perf-cpp"
./bin/olc_coro_tree_perfcpp

# DEMO 4 Binary
echo ""
print_message "DEMO 4: Executing coro-tree: nvtx"
print_message "nsys profile"
./bin/olc_coro_tree_nvtx
