int main() {
    int data[100];
    int result[100];
    
    // Simple parallelizable loop
    for (int i = 0; i < 100; i++) {
        data[i] = i * 2;
    }
    
    // Loop with dependency  
    for (int i = 1; i < 100; i++) {
        data[i] = data[i-1] + 1;
    }
    
    return 0;
}