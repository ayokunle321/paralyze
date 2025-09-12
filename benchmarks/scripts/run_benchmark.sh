#!/bin/bash

# benchmark runner for paralyze vs baseline vs gcc auto-parallel
# usage: ./run_benchmark.sh <kernel_name>

set -e

kernel=$1
polybench_dir="../../PolyBenchC-4.2.1"
paralyze_bin="../../build/paralyze"
results_dir="../results"

if [ -z "$kernel" ]; then
    echo "usage: $0 <kernel_name>"
    echo "example: $0 2mm"
    exit 1
fi

# find the kernel directory
kernel_dir=$(find $polybench_dir -name "$kernel" -type d | head -n 1)
if [ ! -d "$kernel_dir" ]; then
    echo "error: kernel $kernel not found in polybench"
    exit 1
fi

kernel_file="$kernel_dir/$kernel.c"
echo "=== benchmarking $kernel ==="
echo "source: $kernel_file"

# generate paralyze parallelized version
echo "generating paralyze version..."
$paralyze_bin --generate-pragmas $kernel_file
paralyze_file="${kernel_file%.c}_openmp.c"

echo "compiling baseline..."
clang -O3 -I$polybench_dir/utilities -DPOLYBENCH_TIME \
    $kernel_file $polybench_dir/utilities/polybench.c \
    -o ${kernel}_baseline -lm

echo "compiling paralyze version..."
clang -O3 -fopenmp -I$polybench_dir/utilities -DPOLYBENCH_TIME \
    $paralyze_file $polybench_dir/utilities/polybench.c \
    -o ${kernel}_paralyze -lm

echo "compiling gcc auto-parallel..."
gcc-15 -O3 -ftree-parallelize-loops=4 -floop-parallelize-all \
    -I$polybench_dir/utilities -DPOLYBENCH_TIME \
    $kernel_file $polybench_dir/utilities/polybench.c \
    -o ${kernel}_gcc_auto -lm

echo "running baseline..."
baseline_time=$(./${kernel}_baseline 2>&1)
echo "baseline: $baseline_time seconds"

echo "running paralyze..."

export OMP_NUM_THREADS=1
paralyze_1=$(./${kernel}_paralyze 2>&1)
echo "  1 thread: $paralyze_1 seconds"

export OMP_NUM_THREADS=2
paralyze_2=$(./${kernel}_paralyze 2>&1)
echo "  2 threads: $paralyze_2 seconds"

export OMP_NUM_THREADS=4
paralyze_4=$(./${kernel}_paralyze 2>&1)
echo "  4 threads: $paralyze_4 seconds"

export OMP_NUM_THREADS=8
paralyze_8=$(./${kernel}_paralyze 2>&1)
echo "  8 threads: $paralyze_8 seconds"

echo "running gcc auto-parallel (4 threads)..."
gcc_time=$(./${kernel}_gcc_auto 2>&1)
echo "gcc auto: $gcc_time seconds"

# save results
mkdir -p $results_dir
echo "$kernel,$baseline_time,$paralyze_1,$paralyze_2,$paralyze_4,$paralyze_8,$gcc_time" >> $results_dir/results.csv

echo "=== done ==="