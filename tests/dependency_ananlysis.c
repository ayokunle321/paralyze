#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Test 1: Simple safe loop - should be parallelizable
void test_simple_safe() {
    int a[100], b[100], c[100];
    
    for (int i = 0; i < 100; i++) {
        c[i] = a[i] + b[i];  // No dependencies between iterations
    }
}

// Test 2: Loop-carried dependency - unsafe
void test_loop_carried_dependency() {
    int arr[100];
    
    for (int i = 1; i < 100; i++) {
        arr[i] = arr[i-1] + 2;  // arr[i] depends on arr[i-1]
    }
}

// Test 3: Scalar variable dependency - unsafe
void test_scalar_dependency() {
    int data[50];
    int sum = 0;
    
    for (int j = 0; j < 50; j++) {
        sum += data[j];  // sum has read-after-write dependency
        data[j] = sum;
    }
}

// Test 4: Pointer operations - unsafe
void test_pointer_operations() {
    int *ptr = malloc(100 * sizeof(int));
    int *base = ptr;
    
    for (int k = 0; k < 100; k++) {
        *ptr = k;      // Pointer dereference
        ptr++;         // Pointer arithmetic
    }
    
    free(base);
}

// Test 5: Function calls with side effects - unsafe
void test_function_calls() {
    int values[80];
    
    for (int m = 0; m < 80; m++) {
        values[m] = rand();        // rand() has side effects
        printf("%d\n", values[m]); // printf has side effects
    }
}

// Test 6: Safe math functions - potentially safe
void test_math_functions() {
    double input[60];
    double output[60];
    
    for (int n = 0; n < 60; n++) {
        output[n] = sin(input[n]) + cos(input[n]);  // Pure math functions
    }
}

// Test 7: Complex array indexing - unsafe
void test_complex_indexing() {
    int matrix[10][10];
    int indices[100];
    
    for (int i = 0; i < 100; i++) {
        int row = indices[i] % 10;
        int col = (i * 3) % 10;
        matrix[row][col] = i;  // Complex indexing pattern
    }
}

// Test 8: Cross-iteration conflicts
void test_cross_iteration() {
    int buffer[200];
    
    for (int i = 2; i < 198; i++) {
        buffer[i] = buffer[i-2] + buffer[i+1];  // Multiple offset conflicts
    }
}

// Test 9: Nested loops with different safety
void test_nested_loops() {
    int data[20][20];
    
    // Outer loop should be safe, inner has dependency
    for (int i = 0; i < 20; i++) {
        int temp = 0;
        for (int j = 1; j < 20; j++) {
            temp += data[i][j-1];    // Inner loop has dependency
            data[i][j] = temp;
        }
    }
}

// Test 10: Pointer aliasing potential
void test_pointer_aliasing() {
    int array[150];
    int *ptr1 = &array[0];
    int *ptr2 = &array[50];
    
    for (int i = 0; i < 50; i++) {
        *ptr1 = i;           // Could alias with ptr2
        *(ptr2 + i) = i * 2; // Potential aliasing conflict
        ptr1++;
    }
}

// Test 11: Write-only pattern - safe
void test_write_only() {
    int results[300];
    
    for (int i = 0; i < 300; i++) {
        results[i] = i * i * i;  // Only writing, no reads
    }
}

// Test 12: Reduction with local variable - mixed safety
void test_reduction_local() {
    int source[100];
    int total = 0;
    
    for (int i = 0; i < 100; i++) {
        int local = source[i] * 2;  // Local variable - safe
        total += local;             // Reduction - dependency
    }
}

int main() {
    test_simple_safe();
    test_loop_carried_dependency();
    test_scalar_dependency();
    test_pointer_operations();
    test_function_calls();
    test_math_functions();
    test_complex_indexing();
    test_cross_iteration();
    test_nested_loops();
    test_pointer_aliasing();
    test_write_only();
    test_reduction_local();
    
    return 0;
}