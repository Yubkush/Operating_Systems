#include "malloc_2.cpp"
#include <iostream>
#include <cassert>

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

void assert_stat(size_t free_blocks, size_t free_bytes, size_t allocated_blocks, size_t allocated_bytes, size_t meta_data_bytes) {
    assert(_num_free_blocks() == free_blocks);
    assert(_num_free_bytes() == free_bytes);
    assert(_num_allocated_blocks() == allocated_blocks);
    assert(_num_allocated_bytes() == allocated_bytes);
    assert(_num_meta_data_bytes() == meta_data_bytes);
}

void update_stat(size_t *free_blocks, size_t *free_bytes, size_t *allocated_blocks, size_t *allocated_bytes,
                ssize_t fblocks_num, ssize_t fbytes_num, ssize_t ablocks_num, ssize_t abytes_num) {
    *free_blocks += fblocks_num;
    *free_bytes += fbytes_num;
    *allocated_blocks += ablocks_num;
    *allocated_bytes += abytes_num;
}

void test1() {
    assert_stat(0,0,0,0,0);
    size_t free_blocks = 0;
    size_t free_bytes = 0;
    size_t allocated_blocks = 0; 
    size_t allocated_bytes = 0;
    
    int *a = (int*)smalloc(10 * sizeof(int));
    allocated_blocks += 1;
    allocated_bytes += 10 * sizeof(int);
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));
    
    sfree(a);
    free_blocks += 1;
    free_bytes += 10 * sizeof(int);
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));
}

void test2() {
    assert_stat(0,0,0,0,0);
    size_t free_blocks = 0;
    size_t free_bytes = 0;
    size_t allocated_blocks = 0; 
    size_t allocated_bytes = 0;
    
    int *a = (int*)scalloc(10, sizeof(int));
    update_stat(&free_blocks, &free_bytes, &allocated_blocks, &allocated_bytes, 0, 0, 1, 10*sizeof(int));
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));
    
    int *b = (int*)scalloc(5, sizeof(int));
    update_stat(&free_blocks, &free_bytes, &allocated_blocks, &allocated_bytes, 0, 0, 1, 5*sizeof(int));
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));

    sfree(a);
    update_stat(&free_blocks, &free_bytes, &allocated_blocks, &allocated_bytes, 1, 10*sizeof(int), 0, 0);
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));

    int *c = (int*)scalloc(7, sizeof(int));
    update_stat(&free_blocks, &free_bytes, &allocated_blocks, &allocated_bytes, -1, -10*sizeof(int), 0, 0);
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));

    c = (int*)srealloc(c, 13 * sizeof(int));
    update_stat(&free_blocks, &free_bytes, &allocated_blocks, &allocated_bytes, 1, 10*sizeof(int), 1, 13*sizeof(int));
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));
    
    sfree(b);
    update_stat(&free_blocks, &free_bytes, &allocated_blocks, &allocated_bytes, 1, 5*sizeof(int), 0, 0);
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));

    sfree(c);
    update_stat(&free_blocks, &free_bytes, &allocated_blocks, &allocated_bytes, 1, 13*sizeof(int), 0, 0);
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));
}

// malloc a,b free a, calloc c, realloc c to new one, free all
void test3() {
    assert_stat(0,0,0,0,0);
    size_t free_blocks = 0;
    size_t free_bytes = 0;
    size_t allocated_blocks = 0; 
    size_t allocated_bytes = 0;
    
    int *a = (int*)smalloc(10 * sizeof(int));
    update_stat(&free_blocks, &free_bytes, &allocated_blocks, &allocated_bytes, 0, 0, 1, 10*sizeof(int));
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));
    
    int *b = (int*)smalloc(10 * sizeof(int));
    update_stat(&free_blocks, &free_bytes, &allocated_blocks, &allocated_bytes, 0, 0, 1, 10*sizeof(int));
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));
    
    sfree(a);
    update_stat(&free_blocks, &free_bytes, &allocated_blocks, &allocated_bytes, 1, 10*sizeof(int), 0, 0);
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));

    int *c = (int*)scalloc(5, sizeof(int));
    update_stat(&free_blocks, &free_bytes, &allocated_blocks, &allocated_bytes, -1, -10*sizeof(int),0, 0);
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));

    b = (int*)srealloc(b, 12*sizeof(int));
    update_stat(&free_blocks, &free_bytes, &allocated_blocks, &allocated_bytes, 1,10*sizeof(int),1,12*sizeof(int));
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));

    c = (int*)srealloc(c, 10*sizeof(int));
    update_stat(&free_blocks, &free_bytes, &allocated_blocks, &allocated_bytes,0,0,0,0);
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));

    sfree(b);
    update_stat(&free_blocks, &free_bytes, &allocated_blocks, &allocated_bytes, 1, 12*sizeof(int), 0, 0);
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));

    sfree(c);
    update_stat(&free_blocks, &free_bytes, &allocated_blocks, &allocated_bytes, 1, 10*sizeof(int), 0, 0);
    assert_stat(free_blocks,free_bytes,allocated_blocks,allocated_bytes,allocated_blocks*sizeof(metadata_t));
}

int main() {
    test3();
}