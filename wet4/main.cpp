#include "malloc_3.cpp"
#include <iostream>
#include <cassert>

#define MMAP_THRESHOLD (128 * 1024)
#define MAX_ALLOCATION_SIZE (1e8)
#define MIN_SPLIT_SIZE (128)

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

template <typename T>
void populate_array(T *array, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        array[i] = (T)i;
    }
}

template <typename T>
void validate_array(T *array, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        assert((array[i] == (T)i));
    }
}

#define verify_size(base)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        void *after = sbrk(0);                                                                                         \
        assert(_num_allocated_bytes() + aligned_size(_size_meta_data() * _num_allocated_blocks()) ==                  \
                (size_t)after - (size_t)base);                                                                         \
    } while (0)

#define verify_size_with_large_blocks(base, diff)                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        void *after = sbrk(0);                                                                                         \
        assert(diff == (size_t)after - (size_t)base);                                                                 \
    } while (0)

static inline size_t aligned_size(size_t size)
{
    return size;
}

void verify_blocks(size_t allocated_blocks, size_t allocated_bytes, size_t free_blocks, size_t free_bytes) {
    assert(_num_free_blocks() == free_blocks);
    assert(_num_free_bytes() == free_bytes);
    assert(_num_allocated_blocks() == allocated_blocks);
    assert(_num_allocated_bytes() == allocated_bytes);
}

void update_stat(size_t *allocated_blocks, size_t *allocated_bytes, size_t *free_blocks, size_t *free_bytes,
                ssize_t ablocks_num, ssize_t abytes_num, ssize_t fblocks_num, ssize_t fbytes_num) {
    *free_blocks += fblocks_num;
    *free_bytes += fbytes_num;
    *allocated_blocks += ablocks_num;
    *allocated_bytes += abytes_num;
}

void test() {
    verify_blocks(0, 0, 0, 0);
    void *base = sbrk(0);
    char *a = (char *)smalloc(MIN_SPLIT_SIZE + 32);
    char *b = (char *)smalloc(32);
    char *c = (char *)smalloc(32);
    assert(a != nullptr);
    assert(b != nullptr);
    assert(c != nullptr);

    verify_blocks(3, MIN_SPLIT_SIZE + 32 * 3, 0, 0);
    verify_size(base);
    populate_array(b, 32);

    sfree(a);
    sfree(c);
    verify_blocks(3, MIN_SPLIT_SIZE + 32 * 3, 2, MIN_SPLIT_SIZE + 32 * 2);
    verify_size(base);

    char *new_b = (char *)srealloc(b, 64);
    assert(new_b != nullptr);
    assert(new_b == a);
    verify_blocks(2, MIN_SPLIT_SIZE + 32 * 3 + _size_meta_data(), 1, MIN_SPLIT_SIZE + 32 + _size_meta_data());
    verify_size(base);
    validate_array(new_b, 32);

    sfree(new_b);
    verify_blocks(1, MIN_SPLIT_SIZE + 32 * 3 + 2 * _size_meta_data(), 1,
                  MIN_SPLIT_SIZE + 32 * 3 + 2 * _size_meta_data());
    verify_size(base);
}

int main() {
    test();
}