#!/bin/bash

# run benchmarks on all PolyBench linear algebra kernels

KERNELS=(
    "2mm"
    "3mm"
    "atax"
    "bicg"
    "doitgen"
    "gemm"
    "gemver"
    "gesummv"
    "mvt"
    "symm"
    "syr2k"
    "syrk"
    "trmm"
    "trisolv"
    "durbin"
)

echo "kernel,baseline,paralyze_1t,paralyze_2t,paralyze_4t,paralyze_8t,gcc_auto" > ../results/results.csv

for kernel in "${KERNELS[@]}"; do
    echo "Running $kernel..."
    ./run_benchmark.sh $kernel || echo "Failed: $kernel"
done

echo "All benchmarks complete. Results in ../results/results.csv"
