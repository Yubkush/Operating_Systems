#include "malloc_1.cpp"

int main() {
    int *a = (int*)smalloc(sizeof(int) * 10);
    return 0;
}