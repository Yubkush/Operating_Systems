#include <unistd.h>
#include <list>
#define MAX_SIZE (size_t)1e8
#define SYSCALL_FAIL -1

using std::list;

typedef struct MallocMetadata {
    size_t size; // include metadata_t size
    bool is_free;
    void* meta_address; // address of metadata
    void* data_address; // address of allocated data
} metadata_t;

list<metadata_t> free_list;

void* smalloc(size_t size) {
    size_t real_size = size + sizeof(metadata_t);
    if(size == 0 || real_size > MAX_SIZE){
        return NULL;
    }
    for(metadata_t& block: free_list){
        if(block.is_free && block.size >= real_size){
            block.is_free = false;
            return block.data_address;
        }   
    }
    void *meta_addr = sbrk(real_size);
    if(*(ssize_t*)brk == SYSCALL_FAIL) {
        return NULL;
    }
    void *data_addr = (char*)meta_addr + sizeof(metadata_t);
    free_list.push_back((metadata_t){real_size, false, meta_addr, data_addr});
    return data_addr;
}
