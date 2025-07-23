#include <stdlib.h>

// Should be SAFE
void safe_example() {
    int a[10], b[10];
    #pragma omp parallel for simd
    for (int i = 0; i < 10; i++) {
        b[i] = a[i] * 2;
    }
}

// Should be UNSAFE - loop carried dependency
void unsafe_example() {
    int arr[10];
    for (int i = 1; i < 10; i++) {
        arr[i] = arr[i-1] + 1;
    }
}

// Should be UNSAFE - function call
void function_call_example() {
    int data[5];
    for (int i = 0; i < 5; i++) {
        data[i] = rand();
    }
}
