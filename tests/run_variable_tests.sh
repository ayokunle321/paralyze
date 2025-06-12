#!/bin/bash

echo "=== Testing Variable Usage Pattern Detection ==="
echo

cd ../build

echo "Running analysis on variable_patterns.c..."
./statik ../tests/variable_patterns.c

echo ""
echo "Expected results:"
echo "- test_independent_iterations: SAFE TO PARALLELIZE"
echo "- test_loop_carried_dependency: HAS DEPENDENCIES" 
echo "- test_reduction_pattern: HAS DEPENDENCIES (sum variable)"
echo "- test_write_only: SAFE TO PARALLELIZE"
echo "- test_complex_safe: SAFE TO PARALLELIZE (temp is local)"
echo "- test_nested_loops: outer loop SAFE TO PARALLELIZE"
echo "- test_function_calls: HAS DEPENDENCIES (function calls)"