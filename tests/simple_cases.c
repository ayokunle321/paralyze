// Very basic test cases for debugging

void basic_safe_loop() {
    int a[10];
    int b[10];
    
    for (int i = 0; i < 10; i++) {
        b[i] = a[i] + 1;  // Should be safe
    }
}

void basic_unsafe_loop() {
    int arr[10];
    
    for (int i = 1; i < 10; i++) {
        arr[i] = arr[i-1];  // Clear dependency
    }
}

void induction_var_test() {
    int data[100];
    
    for (int counter = 0; counter < 100; counter++) {
        data[counter] = counter * 3;  // counter should be marked as induction var
    }
}