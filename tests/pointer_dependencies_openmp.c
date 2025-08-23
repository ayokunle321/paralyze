#include <stdio.h>
#include <stdlib.h>

// No pointers - should be SAFE
void test_no_pointers() {
    int data[40];
    
    #pragma omp parallel for simd
    for (int i = 0; i < 40; i++) {
        data[i] = i * 2;
    }
}

// Simple pointer dereference - should be UNSAFE
void test_simple_dereference() {
    int arr[30];
    int *ptr = arr;
    
    for (int i = 0; i < 30; i++) {
        *ptr = i;
        ptr++;
    }
}

// Pointer arithmetic - should be UNSAFE
void test_pointer_arithmetic() {
    int data[50];
    int *p = &data[0];
    
    #pragma omp parallel for
    for (int i = 0; i < 50; i++) {
        *(p + i) = i * 3;
    }
}

// Potential aliasing - should be UNSAFE
void test_pointer_aliasing() {
    int buffer[60];
    int *ptr1 = &buffer[0];
    int *ptr2 = &buffer[10];
    
    for (int i = 0; i < 20; i++) {
        *ptr1 = i;
        *ptr2 = i * 2;
        ptr1++;
        ptr2++;
    }
}

// Complex pointer operations - should be UNSAFE
void test_complex_pointers() {
    int *arr = malloc(40 * sizeof(int));
    int *base = arr;
    
    for (int i = 0; i < 40; i++) {
        *arr = i;
        arr = base + (i * 2) % 40;
    }
    
    free(base);
}

// Address-of operations - should be UNSAFE
void test_address_operations() {
    int values[25];
    int *ptrs[25];
    
    for (int i = 0; i < 25; i++) {
        ptrs[i] = &values[i];
        *ptrs[i] = i;
    }
}

int main() {
    test_no_pointers();        // safe - no pointer operations
    test_simple_dereference(); // unsafe - pointer dereference
    test_pointer_arithmetic(); // unsafe - pointer arithmetic
    test_pointer_aliasing();   // unsafe - potential aliasing
    test_complex_pointers();   // unsafe - complex operations
    test_address_operations(); // unsafe - address-of operations
    
    return 0;
}
