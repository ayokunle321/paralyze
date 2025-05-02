# OpenMP Parallelization Analyzer

Static analysis tool to find loops that can be safely parallelized with OpenMP.

## What it does

- Scans C/C++ source files for loops
- Analyzes data dependencies 
- Suggests where to add `#pragma omp parallel for`
- Shows potential speedup estimates

## Building
```bash
mkdir build && cd build
cmake ..
make
```

## Building
```bash
./statik input.c
```