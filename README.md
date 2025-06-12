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