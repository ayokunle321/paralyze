void simple_test() {
    int arr[10];
    int i;
    #pragma omp parallel for simd
    for (i = 0; i < 10; i++) {
        arr[i] = i;
    }
}
