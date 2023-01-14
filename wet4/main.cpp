#include "malloc_2.cpp"
#include <iostream>

void build_arr(int *arr, size_t size) {
    for (size_t i = 0; i < size; i++) {
        arr[i] = i;
    }
}

void print_arr(int *arr, size_t size) {
    for (size_t i = 0; i < size; i++) {
        std::cout << arr[i];
    }
    std::cout << std::endl;
}

int main() {
    int *a = (int*)scalloc(10, sizeof(int));
    int *b = (int*)scalloc(20, sizeof(int));
    int *c = (int*)scalloc(0, sizeof(int));
    int *d = (int*)scalloc((size_t)(1e8+1), sizeof(int));

    print_arr(a, 10);
    print_arr(b, 20);
}