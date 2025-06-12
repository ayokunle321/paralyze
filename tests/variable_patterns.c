#include <stdio.h>

// Test case 1: Simple parallelizable loop (independent iterations)
void test_independent_iterations() {
    int arr[100];
    int result[100];
    
    // This should be marked as safe to parallelize
    for (int i = 0; i < 100; i++) {
        result[i] = arr[i] * 2 + 5;  // No dependencies between iterations
    }
}

// Test case 2: Loop-carried dependency (not parallelizable)
void test_loop_carried_dependency() {
    int arr[100];
    
    // This has a loop-carried dependency - NOT safe to parallelize
    for (int i = 1; i < 100; i++) {
        arr[i] = arr[i-1] + arr[i];  // arr[i] depends on arr[i-1] from previous iteration
    }
}

// Test case 3: Reduction pattern (could be parallelized with special handling)
void test_reduction_pattern() {
    int arr[100];
    int sum = 0;
    
    // Sum reduction - has dependency on 'sum' but could be parallelized
    for (int i = 0; i < 100; i++) {
        sum += arr[i];  // 'sum' is read and written each iteration
    }
}

// Test case 4: Write-only arrays (safe to parallelize)
void test_write_only() {
    int output[100];
    
    for (int i = 0; i < 100; i++) {
        output[i] = i * i;  // Only writing to output[i], no reads
    }
}

// Test case 5: Complex but safe pattern
void test_complex_safe() {
    int a[100], b[100], c[100];
    int temp;
    
    for (int j = 0; j < 100; j++) {
        temp = a[j] + b[j];      // temp is loop-local
        c[j] = temp * 2;         // Only accessing c[j], not other elements
    }
}

// Test case 6: Nested loops with different characteristics
void test_nested_loops() {
    int matrix[10][10];
    int result[10][10];
    
    // Outer loop should be parallelizable
    for (int i = 0; i < 10; i++) {
        // Inner loop accesses different rows
        for (int j = 0; j < 10; j++) {
            result[i][j] = matrix[i][j] + matrix[i][j] * 2;
        }
    }
}

// Test case 7: Function calls (should be marked unsafe)
void helper_function(int x) {
    printf("Value: %d\n", x);
}

void test_function_calls() {
    int arr[50];
    
    for (int k = 0; k < 50; k++) {
        arr[k] = k;
        helper_function(arr[k]);  // Function call makes this unsafe
    }
}

int main() {
    test_independent_iterations();
    test_loop_carried_dependency();
    test_reduction_pattern();
    test_write_only();
    test_complex_safe();
    test_nested_loops();
    test_function_calls();
    
    return 0;
}