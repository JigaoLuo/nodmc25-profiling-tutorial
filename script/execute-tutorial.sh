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
print_message "(1) Building coro-tree"
cmake . -DCMAKE_BUILD_TYPE=Release
make -j4

## Call olc_tree
echo ""
print_message "(2) Executing coro-tree"
./bin/olc_coro_tree

## Upload result file(s)
echo ""
print_message "(3) Uploading result files"
if [ -d "tutorial-result" ]; then
  for result_file in tutorial-result/*.json; do
    file_base_name=$(basename "$result_file")
    curl -T "$result_file" -u "DGjl2FFCi9jzYwK:nodmc2025" "https://tu-dortmund.sciebo.de/public.php/webdav/$file_base_name"
    echo "Uploaded ${result_file}"
  done
else
  echo "Nothing to upload. Did the execution finish successfully?"
fi