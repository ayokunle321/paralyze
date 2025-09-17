# PARALYZE

Automatic loop parallelization with transparent static analysis and OpenMP pragma generation.

---

## Motivation

Automatic parallelization has been a long-standing problem in compiler optimization. While tools like GCC's `-ftree-parallelize-loops` exist, they're often too conservative in practice - parallelizing almost nothing on real code.

After researching existing approaches, I wanted to build a tool that would be more aggressive while staying transparent about its decisions. The goal was to match or beat GCC's auto-parallelizer and show users exactly why each loop can or can't be parallelized.

---

## What It Does

Analyzes C code to find loops that can safely run in parallel. It checks for:
- Variable dependencies between loop iterations
- Array access patterns
- Pointer operations
- Function calls with side effects

Then it either shows you an analysis report or generates OpenMP pragmas for you.

---

## Usage

### Basic Analysis

```bash
./build/paralyze input.c
```

Shows which loops can be parallelized:

![Analysis Output](docs/images/analysis.png)

### Verbose Mode

```bash
./build/paralyze --verbose input.c
```

Shows detailed dependency analysis:

![Verbose Analysis](docs/images/analysis_verbose.png)

### Generate OpenMP Code

```bash
./build/paralyze --generate-pragmas input.c
```

Creates `input_openmp.c` with OpenMP pragmas:

![Pragma Generation](docs/images/pragma_gen.png)

Compile it:
```bash
clang -O3 -fopenmp input_openmp.c -o output
```

Add `--verbose` to see the reasoning:
```bash
./build/paralyze --generate-pragmas --verbose input.c
```

![Verbose Pragma Generation](docs/images/pragma_gen_verbose.png)

---

### Analysis Pipeline

```
C Source Code
    ↓
Clang AST Parser
    ↓
Loop Detection & Bounds Analysis
    ↓
Dependency Analysis
    ├─ Scalar Variables
    ├─ Array Accesses
    ├─ Pointer Operations
    └─ Function Calls
    ↓
Parallelization Decision
    ↓
OpenMP Pragma Generation
    ↓
Annotated Source Code
```
---

## How It Works

Uses Clang to parse C code into an AST, then analyzes each loop:

**Step 1: Find loop variables**  
Identifies the loop counter (usually `i`, `j`, etc.) and how it changes each iteration.

**Step 2: Check scalar variables**  
If a variable is written in one iteration and read in another, the loop can't be parallelized.

**Step 3: Check arrays**  
Loop accessing `A[i]` in iteration `i` is safe. Loop accessing `A[i-1]` has a dependency problem.

**Step 4: Check pointers**  
Conservative here - complex pointer stuff gets flagged as unsafe.

**Step 5: Check function calls**  
Math functions (sin, cos, sqrt) are safe. Everything else is assumed unsafe.

### Key Decisions

**Only parallelize outer loops**  
When you parallelize an outer loop, the inner loops run normally inside each thread. No point in nested parallelism.

**Conservative by default**  
Better to miss an opportunity than introduce a race condition.

**Confidence scoring**
Each pragma gets a confidence score (0-100%) based on loop complexity, dependencies, and access patterns. Higher scores mean safer parallelization.

---

## Benchmark Results

Tested on 15 PolyBench linear algebra kernels:
- **Baseline**: `clang -O3` (single-threaded)
- **PARALYZE**: `clang -O3 -fopenmp` (4 threads)
- **GCC Auto**: `gcc-15 -O3 -ftree-parallelize-loops=4`

### Analysis

![Analysis Results](docs/images/results_analysis.png)

### Speedup

![Speedup Comparison](benchmarks/results/speedup_comparison.png)

**Results:**
- 6 out of 15 kernels got real speedup (>1.5x)
- Best case: 6.45x faster on triangular solver
- Vector operations parallelize well
- Matrix operations have OpenMP overhead issues
- GCC auto-parallelization barely did anything

---

## Limitations

- For small loops, the cost of spawning threads can exceed the benefit (saw this in matmul benchmarks)
- PolyBench uses macros that turn arrays into pointers - the tool can't see through this, so it misses some dependencies
- Flags complex pointer operations as unsafe even when they might be fine

---

## Building

### Requirements
- Clang/LLVM 15+
- CMake 3.15+
- C++17 compiler
- gcc-15 (to run benchmarks)

### Build

```bash
git clone https://github.com/ayokunle321/paralyze.git
cd paralyze
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Binary: `build/paralyze`

---

## Project Structure

```
paralyze/
├── include/analyzer/
│   ├── ASTVisitor.h              
│   ├── LoopVisitor.h             # Loop detection
│   ├── DependencyAnalyzer.h      # Dependency checks
│   ├── ArrayDependencyAnalyzer.h
│   ├── PointerAnalyzer.h
│   ├── FunctionCallAnalyzer.h
│   ├── PragmaGenerator.h         # OpenMP generation
│   └── SourceAnnotator.h         # Code annotation
├── src/                          # Implementations
├── benchmarks/
│   ├── scripts/                  # Benchmark runners
│   └── results/                  # Performance data
└── README.md
```

---

## Future Work

- Extend support to C++
- Better model to predict when parallelization is worth it
- Better array dependency analysis

---

## Contact

**ayokunle321@gmail.com** or open an issue on GitHub