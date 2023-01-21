#include "my_stdlib.h"
#include <sys/wait.h>
#include <catch2/catch_test_macros.hpp>

#include <unistd.h>

#define MAX_ALLOCATION_SIZE (1e8)
#define MMAP_THRESHOLD (128 * 1024)
#define MIN_SPLIT_SIZE (128)

static inline size_t aligned_size(size_t size)
{
    return size;
}

#define verify_blocks(allocated_blocks, allocated_bytes, free_blocks, free_bytes)                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        REQUIRE(_num_allocated_blocks() == allocated_blocks);                                                          \
        REQUIRE(_num_allocated_bytes() == aligned_size(allocated_bytes));                                              \
        REQUIRE(_num_free_blocks() == free_blocks);                                                                    \
        REQUIRE(_num_free_bytes() == aligned_size(free_bytes));                                                        \
        REQUIRE(_num_meta_data_bytes() == aligned_size(_size_meta_data() * allocated_blocks));                         \
    } while (0)

#define verify_size(base)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        void *after = sbrk(0);                                                                                         \
        REQUIRE(_num_allocated_bytes() + aligned_size(_size_meta_data() * _num_allocated_blocks()) ==                  \
                (size_t)after - (size_t)base);                                                                         \
    } while (0)

#define verify_size_with_large_blocks(base, diff)                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        void *after = sbrk(0);                                                                                         \
        REQUIRE(diff == (size_t)after - (size_t)base);                                                                 \
    } while (0)

TEST_CASE("cookie overflow", "[malloc3]")
{
    int res = fork();
    if(res == 0) {
        verify_blocks(0, 0, 0, 0);

        void *base = sbrk(0);
        char *a = (char *)smalloc(10);
        char *b = (char *)smalloc(10);
        REQUIRE(a != nullptr);
        REQUIRE(b != nullptr);

        verify_blocks(2, 20, 0, 0);
        verify_size(base);
        
        strcpy(a, "cookie destroyer");

        sfree(a);
        sfree(b);
        exit(0);
    }
    int status = 0;
    waitpid(res, &status, 0);
    REQUIRE((WIFEXITED(status)));
	// returns only 8 lowest bits
    REQUIRE((0xef == WEXITSTATUS(status)));
}