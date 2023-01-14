#include "malloc_2.cpp"

void build_arr(int *arr, size_t size) {
    for (size_t i = 0; i < size; i++) {
        arr[i] = i;
    }
}

int main() {
    int *a = (int*)smalloc(sizeof(int) * 10);
    int *b = (int*)smalloc(sizeof(int) * 20);
    int *c = (int*)smalloc(0);
    int *d = (int*)smalloc((size_t)(1e8+1));

    build_arr(a, 10);
    build_arr(b, 20);
}