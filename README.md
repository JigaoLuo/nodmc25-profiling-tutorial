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