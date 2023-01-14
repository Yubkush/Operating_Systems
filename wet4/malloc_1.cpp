#include <unistd.h>
#define MAX_SIZE (size_t)1e8
#define SYSCALL_FAIL -1

void* smalloc(size_t size) {
    if(size == 0 || size > MAX_SIZE){
        return NULL;
    }
    void *brk = sbrk(size);
    if(*(ssize_t*)brk == SYSCALL_FAIL){
        return NULL;
    }
    return brk;
}
