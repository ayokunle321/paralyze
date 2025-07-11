#!/bin/bash
echo "=== Testing OpenMP Pragma Generation ==="
echo
cd ../build
echo "Running pragma generation analysis on pragma_generation.c..."
echo
./statik ../tests/pragma_generation.c
echo ""
echo "=========================================="
echo "Expected Results:"
echo "=========================================="
echo "SHOULD GENERATE PRAGMAS:"
echo "- test_simple_parallel: #pragma omp parallel for (HIGH confidence)"
echo "- test_simd_candidate: #pragma omp parallel for simd (HIGH confidence)"
echo "- test_complex_safe: #pragma omp parallel for (MEDIUM confidence)"
echo "- test_hot_computation: #pragma omp parallel for simd (VERY HIGH confidence)"
echo ""
echo "SHOULD NOT GENERATE PRAGMAS:"
echo "- test_unsafe_loop: loop-carried dependency detected"
echo "- test_with_function_calls: side effects from printf()"
echo ""
echo "CONFIDENCE EXPECTATIONS:"
echo "- Simple array operations: High confidence"
echo "- Math-heavy loops: Very high confidence" 
echo "- Complex but safe patterns: Medium confidence"
echo "- Loops with side effects: No pragma or low confidence"