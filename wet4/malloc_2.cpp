#include <unistd.h>
#include <cstring>
#define MAX_SIZE (size_t)1e8
#define SYSCALL_FAIL -1

using std::memset;
using std::memmove;

typedef struct MallocMetadata {
    size_t size; // include metadata_t size
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
} metadata_t;

static metadata_t list_head = {0, false, nullptr, nullptr};

void* smalloc(size_t size) {
    size_t real_size = size + sizeof(metadata_t);
    if(size == 0 || real_size > MAX_SIZE){
        return NULL;
    }
    metadata_t *tmp = list_head.next;
    metadata_t *last = nullptr;
    while(tmp != nullptr){
        if(tmp->is_free && tmp->size >= real_size){
            tmp->is_free = false;
            return (char*)tmp + sizeof(metadata_t);
        }
        if(tmp->next == nullptr){last = tmp;}
        tmp = tmp->next; 
    }
    void *res = sbrk(real_size);
    if(*(ssize_t*)res == SYSCALL_FAIL) {
        return NULL;
    }
    tmp = (metadata_t*)res;
    tmp->size = real_size;
    tmp->is_free = false;
    tmp->next = nullptr;
    if(list_head.next == nullptr){last = &list_head;} // if this is the first node
    tmp->prev = last;
    last->next = tmp;
    return (char*)tmp + sizeof(metadata_t);
}

void* scalloc(size_t num, size_t size) {
    void *addr = smalloc(size * num);
    if(addr == nullptr) {
        return NULL;
    }
    memset(addr, 0, size * num);
    return addr;
}

void sfree(void* p) {
    if(p == nullptr)
        return;
    metadata_t *p_meta = (metadata_t*)p - 1;
    p_meta->is_free = true;
}

void* srealloc(void* oldp, size_t size) {
    size_t real_size = size + sizeof(metadata_t);
    if(size == 0 || real_size > MAX_SIZE){
        return NULL;
    }
    if(oldp == nullptr){
        return smalloc(size);
    }
    metadata_t *p_meta = (metadata_t*)oldp - 1;
    if(p_meta->size < real_size){
        return oldp;
    }
    void* newp = smalloc(real_size);
    if(newp == nullptr){return NULL;}
    memmove(newp, oldp, size);
    sfree(oldp);
    return newp;
}