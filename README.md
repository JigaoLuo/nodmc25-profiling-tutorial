# Demo

We will showcase demos of various profiling tools. Special thanks to Jan for generously providing this tutorial during the NoDMC workshop and for developing `perf-cpp`. 
If you encounter compilation errors or have questions, please file an issue on [the perf-cpp GitHub page](https://github.com/jmuehlig/perf-cpp).

The demo will be a OLC-Btree with insert and look operations. This is also the same code from the tutorial during NoDMC.

## Build

```bash
cmake . -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j4
```

## Demo 1: `perf`

```bash
$ perf stat ./bin/olc_coro_tree_perf
$ perf record ./bin/olc_coro_tree_perf
$ perf report
```

## Demo 2: `perfevent`

```bash
$ ./bin/olc_coro_tree_perfevent
```

## Demo 3: `perf-cpp`

```bash
$ ./bin/olc_coro_tree_perfcpp
```

## Demo 4: NSYS

```bash
$ nsys profile ./bin/olc_coro_tree_nvtx
# $ nsys profile  --event-sample='system-wide' --os-events='0,1,2,3,4,5,6,7,8' --cpu-core-events='1,2,3' ./bin/olc_coro_tree_nvtx  
$ scp # to your local
$ # open it via local nsys-GUI
```

---

# Understanding Application Performance on Modern Hardware: Profiling Foundations and Advanced Techniques 

This is the project related to the [NoDMC'25](https://sites.google.com/view/nodmcbtw2025) tutorial [Understanding Application Performance on Modern Hardware: Profiling Foundations and Advanced Techniques ](https://dbis.cs.tu-dortmund.de/storages/dbis-cs/r/papers/2025/prefetching-tutorial/profiling-tutorial-submission.pdf).

## Usage

**Option 1:** The following script will download this repo, build and execute it, and upload the result file to our scoreboard.

```bash
curl https://raw.githubusercontent.com/jmuehlig/nodmc25-profiling-tutorial/refs/heads/main/script/download-and-execute-tutorial.sh | sh
```

**Option 2:** Clone this repo and execute the tutorial script. The tutorial script will build and execute the benchmark and pload the result file to our scoreboard.

```bash
# Get the code
git clone https://github.com/jmuehlig/nodmc25-profiling-tutorial.git 
cd nodmc25-profiling-tutorial

# Execute
sh script/execute-tutorial.sh
```

**Option 3:** Clone the repo and build and execute by yourself.
```bash
# Get the code
git clone https://github.com/jmuehlig/nodmc25-profiling-tutorial.git 
cd nodmc25-profiling-tutorial

# Build
cmake .
make

# Execute binary
./bin/olc_coro_tree
```