#include <stdio.h>

// Simple nested loops - should be SAFE
void test_simple_nested() {
    int matrix[10][10];
    
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            matrix[i][j] = i * j;
        }
    }
}

// Cross-iteration stride conflict - should be UNSAFE
void test_stride_conflict() {
    int data[100];
    
    for (int i = 0; i < 95; i++) {
        data[i+3] = data[i] + 1;
    }
}

// Nested with inner dependency - should be UNSAFE
void test_nested_inner_dependency() {
    int arr[20][20];
    
    for (int i = 0; i < 20; i++) {
        for (int j = 1; j < 20; j++) {
            arr[i][j] = arr[i][j-1] + 1;
        }
    }
}

// Cross-iteration read-after-write - should be UNSAFE
void test_cross_read_write() {
    int buffer[60];
    
    for (int i = 2; i < 58; i++) {
        buffer[i] = buffer[i-2] + buffer[i+1];
    }
}

// Nested independent loops - should be SAFE
void test_nested_independent() {
    int a[15][15], b[15][15], c[15][15];
    
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            c[i][j] = a[i][j] + b[i][j];
        }
    }
}

// Multiple offset conflicts - should be UNSAFE
void test_multiple_offsets() {
    int values[80];
    
    for (int i = 5; i < 75; i++) {
        values[i] = values[i-3] + values[i+2];
    }
}

// Nested with outer dependency - should be UNSAFE
void test_nested_outer_dependency() {
    int grid[12][12];
    int sum = 0;
    
    for (int i = 0; i < 12; i++) {
        sum += i;
        for (int j = 0; j < 12; j++) {
            grid[i][j] = sum;
        }
    }
}

// Cross-iteration same index - should be UNSAFE
void test_cross_same_index() {
    int array[50];
    
    for (int i = 0; i < 50; i++) {
        array[i] = array[i] * 2 + 1;
    }
}

int main() {
    test_simple_nested();           // safe - independent nested
    test_stride_conflict();         // unsafe - stride pattern
    test_nested_inner_dependency(); // unsafe - inner loop dependency
    test_cross_read_write();        // unsafe - cross-iteration r/w
    test_nested_independent();      // safe - nested independent ops
    test_multiple_offsets();        // unsafe - multiple offset conflicts
    test_nested_outer_dependency(); // unsafe - outer loop dependency
    test_cross_same_index();        // unsafe - same index conflict
    
    return 0;
}