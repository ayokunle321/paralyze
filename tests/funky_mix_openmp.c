#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void chaos_function() {
    int data[100];
    int *ptr = malloc(50 * sizeof(int));
    double results[100];
    int accumulator = 0;
    
    for (int i = 0; i < 50; i++) {
        data[i] = rand();             
        ptr[i] = data[i] * 2;          
        accumulator += ptr[i];         // Scalar reduction
        results[i] = sin(data[i]);     // Math function (safe)
        data[i+25] = accumulator;      
    }
    
    free(ptr);
}

// Nested loops with mixed safety patterns
void nested_chaos() {
    int matrix[20][20];
    int temp_sum = 0;
    
    for (int i = 0; i < 20; i++) {
        int local_var = i * 2;         // Local variable (safe)
        
        for (int j = 1; j < 20; j++) {
            matrix[i][j] = matrix[i][j-1] + local_var;  
            temp_sum += matrix[i][j];                  
            
            if (j % 2 == 0) {
                printf("Value: %d\n", matrix[i][j]);    // Function call
            }
        }
    }
}

// Pointer aliasing 
void pointer_array_mix() {
    int buffer[200];
    int *start = &buffer[0];
    int *middle = &buffer[100];
    
    for (int i = 0; i < 100; i++) {
        buffer[i] = i;                 
        *(start + i) = i * 2;          
        middle[i] = buffer[i] + 1;     
        
        if (i > 5) {
            buffer[i-3] = *(middle + i - 5);  // Complex dependency pattern
        }
    }
}

int main() {
    chaos_function();      // unsafe
    nested_chaos();        // unsafe
    pointer_array_mix();   // unsafe
    return 0;
}
