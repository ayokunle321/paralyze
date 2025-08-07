// Testing array access dependency patterns

#include <stdio.h>

// Independent arrays - should be SAFE
void test_independent_access() {
    int src[50], dst[50];
    
    for (int i = 0; i < 50; i++) {
        dst[i] = src[i] * 2;
    }
}

// Same index read/write - should be UNSAFE
void test_same_index_conflict() {
    int data[40];
    
    for (int i = 0; i < 40; i++) {
        data[i] = data[i] + 1;
    }
}

// Loop-carried dependency - should be UNSAFE
void test_loop_carried() {
    int array[60];
    
    for (int i = 1; i < 60; i++) {
        array[i] = array[i-1] + i;
    }
}

// Write-only access - should be SAFE
void test_write_only() {
    int results[30];
    
    for (int i = 0; i < 30; i++) {
        results[i] = i * i;
    }
}

// Constant offset - should be UNSAFE
void test_constant_offset() {
    int data[50];
    
    for (int i = 0; i < 45; i++) {
        data[i+3] = data[i] + 1;
    }
}

// Multiple independent arrays - should be SAFE
void test_multiple_arrays() {
    int a[25], b[25], c[25];
    
    for (int i = 0; i < 25; i++) {
        c[i] = a[i] + b[i];
    }
}

int main() {
    test_independent_access();   // safe - different arrays
    test_same_index_conflict();  // unsafe - same index r/w
    test_loop_carried();         // unsafe - backward dependency
    test_write_only();           // safe - write only
    test_constant_offset();      // unsafe - stride pattern
    test_multiple_arrays();      // safe - independent operations
    
    return 0;
}