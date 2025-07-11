#include <stdio.h>
#include <math.h>

// Test 1: Simple parallel for candidate - should get high confidence
void test_simple_parallel() {
    int a[1000], b[1000], c[1000];
    
    for (int i = 0; i < 1000; i++) {
        c[i] = a[i] + b[i];  // Simple, independent operations
    }
}

// Test 2: SIMD candidate - should get parallel for simd pragma
void test_simd_candidate() {
    double data[500];
    double results[500];
    
    for (int j = 0; j < 500; j++) {
        results[j] = data[j] * data[j] + sin(data[j]);  // Arithmetic + math function
    }
}

// Test 3: Complex but safe - should get medium confidence
void test_complex_safe() {
    float matrix[100][100];
    float output[100][100];
    int temp;
    
    for (int i = 0; i < 100; i++) {
        temp = i * 2;  // Private variable
        for (int j = 0; j < 100; j++) {
            output[i][j] = matrix[i][j] + temp;
        }
    }
}

// Test 4: Unsafe - should not generate pragma
void test_unsafe_loop() {
    int arr[50];
    
    for (int k = 1; k < 50; k++) {
        arr[k] = arr[k-1] + k;  // Loop-carried dependency
    }
}

// Test 5: Function calls - should get lower confidence or no pragma
void test_with_function_calls() {
    int values[80];
    
    for (int m = 0; m < 80; m++) {
        values[m] = m;
        printf("Value: %d\n", values[m]);  // Side effect
    }
}

// Test 6: Hot loop with many operations - should get high confidence
void test_hot_computation() {
    double input[2000];
    double output[2000];
    
    for (int n = 0; n < 2000; n++) {
        double temp = input[n];
        temp = temp * temp;
        temp = temp + (temp * 0.5);
        temp = sqrt(temp);
        output[n] = temp;  // Many arithmetic operations
    }
}

int main() {
    test_simple_parallel();
    test_simd_candidate();
    test_complex_safe();
    test_unsafe_loop();
    test_with_function_calls();
    test_hot_computation();
    
    return 0;
}