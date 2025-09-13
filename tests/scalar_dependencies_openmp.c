#include <stdio.h>

// Basic dependency - should be unsafe
void test_accumulator() {
    int data[50];
    int sum = 0;
    
    for (int i = 0; i < 50; i++) {
        sum += data[i];  // sum depends on previous iterations
        data[i] = sum;
    }
}

// Simple safe case
void test_simple_safe() {
    int arr[100];
    
    #pragma omp parallel for simd
    for (int i = 0; i < 100; i++) {
        arr[i] = i * 2;  // just using loop variable
    }
}

// Local variable test
void test_local_var() {
    int results[60];
    
    #pragma omp parallel for simd
    for (int i = 0; i < 60; i++) {
        int temp = i * 3;
        results[i] = temp + 1;  // temp is local each iteration
    }
}

// Multiple scalar dependencies
void test_multiple_deps() {
    int array[30];
    int x = 1, y = 2;
    
    for (int i = 0; i < 30; i++) {
        array[i] = x + y;
        x = y;        // creates dependency
        y = x + i;    // creates dependency
    }
}

// Read-only external variable
void test_read_only() {
    int source[80], output[80];
    int multiplier = 5;
    
    #pragma omp parallel for simd
    for (int i = 0; i < 80; i++) {
        output[i] = source[i] * multiplier;  // multiplier never changes
    }
}

// Conditional dependency
void test_conditional() {
    int values[40];
    int flag = 0;
    
    for (int i = 0; i < 40; i++) {
        if (i % 2 == 0) {
            flag = 1;
        }
        values[i] = flag;  // flag carries between iterations
    }
}


// Index manipulation
void test_index_change() {
    int arr[50];
    int idx = 0;
    
    for (int i = 0; i < 25; i++) {
        arr[idx] = i;
        idx = (idx + 2) % 50;  // idx changes each iteration
    }
}

int main() {
    test_accumulator();    // unsafe - accumulation
    test_simple_safe();    // safe - just loop var
    test_local_var();      // safe - local only
    test_multiple_deps();  // unsafe - x,y dependencies  
    test_read_only();      // safe - read only
    test_conditional();    // unsafe - flag dependency
    test_index_change();   // unsafe - index dependency
    
    return 0;
}
