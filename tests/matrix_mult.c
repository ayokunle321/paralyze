#include <stdio.h>
#include <math.h>

// Matrix multiplication - outer loops safe, inner has reduction
void matrix_multiply() {
    int A[20][20], B[20][20], C[20][20];
    
    for (int i = 0; i < 20; i++) {        
        for (int j = 0; j < 20; j++) {    
            C[i][j] = 0;
            for (int k = 0; k < 20; k++) {
                C[i][j] += A[i][k] * B[k][j];  
            }
        }
    }
}

// Vector operations - mix of safe and unsafe patterns
void vector_operations() {
    double a[1000], b[1000], c[1000];
    
    // Vector addition - should be SAFE
    for (int i = 0; i < 1000; i++) {
        c[i] = a[i] + b[i];  
    }
    
    // Dot product reduction - should be UNSAFE
    double sum = 0.0;
    for (int i = 0; i < 1000; i++) {
        sum += a[i] * b[i];  // sum has read-after-write dependency
    }
}

// Image blur filter - should be SAFE
void image_processing() {
    int image[50][50];
    int filtered[50][50];
    
    for (int i = 1; i < 49; i++) {        // safe - independent rows
        for (int j = 1; j < 49; j++) {    // safe - independent columns
            filtered[i][j] = (image[i-1][j] + image[i+1][j] + 
                             image[i][j-1] + image[i][j+1] + 
                             image[i][j]) / 5;  // Read from input, write to output
        }
    }
}

int main() {
    matrix_multiply();    // mixed safety
    vector_operations();  // mixed safety
    image_processing();   // safe
    return 0;
}