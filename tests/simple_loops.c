#include <stdio.h>

int main() {
    int i, j;
    int arr[100];
    int sum = 0;
    
    // Simple for loop
    for (i = 0; i < 100; i++) {
        arr[i] = i * 2;
    }
    
    // Nested for loops
    for (i = 0; i < 10; i++) {
        for (j = 0; j < 10; j++) {
            sum += i + j;
        }
    }
    
    // While loop
    i = 0;
    while (i < 50) {
        sum += arr[i];
        i++;
    }
    
    // Do-while loop
    j = 10;
    do {
        sum += j;
        j--;
    } while (j > 0);
    
    return sum;
}

void helper_function() {
    int k;
    
    // Loop in another function
    for (k = 0; k < 5; k++) {
        printf("Helper: %d\n", k);
    }
}