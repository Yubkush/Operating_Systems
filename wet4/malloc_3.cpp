#include <unistd.h>
#include <cstring>
#include <list>
#include <random>
#include <limits>
#define MAX_SIZE (size_t)1e8
#define MMAP_THRESH (size_t)128*1024
#define SYSCALL_FAIL -1
#define meta_s sizeof(metadata_t)

using std::memset;
using std::memmove;
using std::list;

enum class FreeBlocks {not_found, small, big};

typedef struct Stats {
    size_t num_free_blocks;
    size_t num_free_bytes;
    size_t num_allocated_blocks;
    size_t num_allocated_bytes;
    // size_t num_meta_data_bytes = num_allocated_blocks * meta_s
    // size_t size_meta_data = meta_s
} stats_t;

stats_t stats = {0, 0, 0, 0};

typedef struct MallocMetadata {
    size_t size; // not including metadata_t size
    bool is_free;
    __int32_t cookies;
    MallocMetadata* addr_next;
    MallocMetadata* addr_prev;
    MallocMetadata* size_next;
    MallocMetadata* size_prev;
} metadata_t;

class AddrCondition {
    public:
        AddrCondition() = default;
        ~AddrCondition() = default;
        bool operator()(metadata_t *m1, metadata_t *m2) const {
            return m1 < m2;
        }
};

static bool cmp_metadata(metadata_t *m1, metadata_t *m2) {
    if(m1->size < m2->size)
        return true;
    else if(m1->size > m2->size)
        return false;
    return m1 < m2;
}

// set random cookies
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
const __int32_t cookies = dist(gen);


class MemManage {
    public:
        metadata_t addr_head;
        metadata_t free_head;
        metadata_t *wilderness;
        MemManage(): wilderness(nullptr) {
            addr_head = {0, false, cookies, nullptr, nullptr, nullptr, nullptr};
            free_head = {0, false, cookies, nullptr, nullptr, nullptr, nullptr};
        }

        ~MemManage() = default;
        void createBlock(size_t size, metadata_t *addr) {
            metadata_t* tmp = addr_head.addr_next;
            // insert to addr list
            if(tmp == nullptr) { // addr list is empty
                addr_head.addr_next = addr;
                addr->addr_prev = nullptr;
                addr->addr_next = nullptr;
            }
            else{ // not empty
                while(tmp->addr_next != nullptr) {
                    tmp = tmp->addr_next;
                }
                tmp->addr_next = addr;
                addr->addr_prev = tmp;
                addr->addr_next = nullptr;
            }
            // update block 
            addr->is_free = false;
            addr->size = size;
            wilderness = addr;
            // update stats
            stats.num_allocated_blocks++;
            stats.num_allocated_bytes += size;
        }

        void insertFreeBlock(metadata_t *addr) {
            metadata_t *tmp = free_head.size_next;
            // insert to size list
            if(tmp == nullptr) { // size list is empty
                free_head.size_next = addr;
                addr->size_prev = nullptr;
                addr->size_next = nullptr;
            }
            else{ // not empty
                while(tmp->size_next != nullptr && cmp_metadata(tmp, addr)) {
                    tmp = tmp->size_next;
                }
                addr->size_next = tmp->size_next;
                if(tmp->size_next != nullptr) tmp->size_next->size_prev = addr;
                tmp->size_next = addr;
                addr->size_prev = tmp;
            }
            addr->is_free = true;
            stats.num_free_blocks++;
            stats.num_free_bytes += addr->size;
        }

        void removeFreeBlock(metadata_t *block) {
            if(block->size_prev == nullptr) { // first node in list
                free_head.size_next = block->size_next;
            }
            else if(block->size_next == nullptr) { // last node in list, not first one
                block->size_prev->size_next = nullptr;
            }
            else {
                block->size_prev->size_next = block->size_next;
                block->size_next->size_prev = block->size_prev;
            }
            block->is_free = false;
            stats.num_free_blocks--;
            stats.num_free_bytes -= block->size;
        }

        FreeBlocks findFreeBlock(size_t memory ,metadata_t* result){
            metadata_t *tmp = free_head.size_next;
            while(tmp != nullptr && tmp->size < memory) {
                tmp = tmp->size_next;
            }
            if(tmp == nullptr)
                return FreeBlocks::not_found;
            if(tmp->size == memory){
                result = tmp;
                return FreeBlocks::small;
            }
            metadata_t *frag = tmp;
            while(tmp != nullptr && tmp->size < memory + 128 + meta_s) {
                tmp = tmp->size_next;
            }
            if(tmp == nullptr) {
                result = frag;
                return FreeBlocks::small;
            }
            result = tmp;
            return FreeBlocks::big;
        }

        void splitBlock(size_t memory ,metadata_t* block) {
            metadata_t *new_block = (metadata_t*)((char*)block + memory + meta_s);
            new_block->size = block->size - meta_s - memory;
            block->size = memory;
            // update addr list
            new_block->addr_next = block->addr_next;
            new_block->addr_prev = block;
            block->addr_next = new_block;
            // update free list and stats free related
            if(block->is_free)
                removeFreeBlock(block);
            insertFreeBlock(new_block);
            // assume block is not free, otherwise insert it to free list
            
            stats.num_allocated_blocks++;
            stats.num_allocated_bytes -= meta_s;
            // update wilderness
            if(block == wilderness) {
                wilderness = new_block;
            }
        }

        void mergeBlocks(metadata_t* block) {
            // update wilderness
            if(block->addr_next == wilderness) {
                wilderness = block;
            }
            if(block->is_free) {
                removeFreeBlock(block);
            }
            if(block->addr_next->is_free) {
                removeFreeBlock(block->addr_next);
            }
            block->size += block->addr_next->size + meta_s;
            if(block->is_free && block->addr_next->is_free) {
                insertFreeBlock(block);
            }
            // remove node from addr list
            block->addr_next = block->addr_next->addr_next;
            block->addr_next->addr_prev = block;
            // update stats
            stats.num_allocated_blocks--;
            stats.num_allocated_bytes += meta_s;
        }
};

MemManage block_list;

void* smalloc(size_t size) {
    size_t real_size = size + meta_s;
    if(size == 0 || real_size > MAX_SIZE){
        return NULL;
    }
    metadata_t *block = nullptr;
    FreeBlocks result = block_list.findFreeBlock(size, block);
    switch (result) {
        case FreeBlocks::not_found: {
            if(block_list.wilderness->is_free) {
                void *res = sbrk(size - block_list.wilderness->size);
                if(*(ssize_t*)res == SYSCALL_FAIL) {
                    return NULL;
                }
                block_list.wilderness->size += size - block_list.wilderness->size;
                block_list.removeFreeBlock(block_list.wilderness);
                stats.num_allocated_bytes += size - block_list.wilderness->size;
                return (char*)block_list.wilderness + meta_s;
            }
            void *res = sbrk(real_size);
            if(*(ssize_t*)res == SYSCALL_FAIL) {
                return NULL;
            }
            block_list.createBlock(size, (metadata_t*)res);
            return (char*)res + meta_s;
        }
        case FreeBlocks::small: {
            block_list.removeFreeBlock(block);
            return (char*)block + meta_s;
        }
        case FreeBlocks::big: {
            block_list.splitBlock(size, block);
            return (char*)block + meta_s;
        }
        default:
            return NULL;
    }
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
    metadata_t *p_meta = (metadata_t*)p - 1;
    if(p == nullptr || p_meta->is_free == true)
        return;
    block_list.insertFreeBlock(p_meta);
    // merge free blocks ***chalange 2
    if(p_meta->addr_next != nullptr && p_meta->addr_next->is_free) {
        block_list.mergeBlocks(p_meta);
    }
    if(p_meta->addr_prev != nullptr && p_meta->addr_prev->is_free) {
        block_list.mergeBlocks(p_meta->addr_prev);
    }
}

static void splitRealloc(size_t size ,metadata_t* to_split) {
    if(to_split->size > size + 128 + meta_s) {
        block_list.splitBlock(size, to_split);
    }
}

void* srealloc(void* oldp, size_t size) {
    if(size == 0 || size + meta_s > MAX_SIZE){
        return NULL;
    }
    if(oldp == nullptr){
        return smalloc(size);
    }
    metadata_t *p_meta = (metadata_t*)oldp - 1;
    if(p_meta->size >= size){   // 1.a
        splitRealloc(size, (metadata_t*)oldp);
        return oldp;
    }
    else if(p_meta->addr_prev != nullptr && p_meta->addr_prev->is_free) {   // 1.b
        if(p_meta == block_list.wilderness){    // wilderness case
            metadata_t *prev = p_meta->addr_prev;
            block_list.mergeBlocks(prev);
            // enlarging the wilderness block
            if(block_list.wilderness->size < size) {
                void *res = sbrk(size - block_list.wilderness->size);
                if(*(ssize_t*)res == SYSCALL_FAIL) {
                    return NULL;
                }
                block_list.wilderness->size += size - block_list.wilderness->size;
                stats.num_allocated_bytes += size - block_list.wilderness->size;
            }
            else {
                splitRealloc(size, prev);
            }
            memmove((char*)prev + meta_s, oldp, p_meta->size);
            return (char*)prev + meta_s;
        }
        else if(p_meta->addr_prev->size + p_meta->size >= size) {
            metadata_t *prev = p_meta->addr_prev;
            block_list.mergeBlocks(p_meta->addr_prev);
            splitRealloc(size, prev);
            memmove((char*)prev + meta_s, oldp, p_meta->size);
            return (char*)prev + meta_s;
        }
    }
    else if(p_meta == block_list.wilderness) {  // 1.c
        void *res = sbrk(size - block_list.wilderness->size);
        if(*(ssize_t*)res == SYSCALL_FAIL) {
            return NULL;
        }
        block_list.wilderness->size += size - block_list.wilderness->size;
        stats.num_allocated_bytes += size - block_list.wilderness->size;
        memmove((char*)block_list.wilderness + meta_s, oldp, p_meta->size);
        return (char*)block_list.wilderness + meta_s;
    }
    else if(p_meta->addr_next != nullptr && p_meta->addr_next->is_free && p_meta->addr_next->size + p_meta->size >= size) {   // 1.d
        block_list.mergeBlocks(p_meta);
        splitRealloc(size, p_meta);
        memmove((char*)p_meta + meta_s, oldp, p_meta->size);
        return (char*)p_meta + meta_s;
    }
    else if(p_meta->addr_next != nullptr && p_meta->addr_next->is_free && 
            p_meta->addr_prev != nullptr && p_meta->addr_prev->is_free && 
            p_meta->addr_next->size + p_meta->addr_prev->size + p_meta->size >= size) {   // 1.e
        metadata_t *prev = p_meta->addr_prev;
        block_list.mergeBlocks(p_meta);
        block_list.mergeBlocks(p_meta->addr_prev);
        memmove((char*)prev + meta_s, oldp, p_meta->size);
        return (char*)prev + meta_s;
    }
    else if(p_meta->addr_next != nullptr && p_meta->addr_next->is_free && p_meta->addr_next == block_list.wilderness) {    // 1.f
        if(p_meta->addr_prev != nullptr && p_meta->addr_prev->is_free) { // 1.f.i
            block_list.mergeBlocks(p_meta->addr_prev);
        }
        block_list.mergeBlocks(p_meta);
        void *res = sbrk(size - block_list.wilderness->size);
        if(*(ssize_t*)res == SYSCALL_FAIL) {
            return NULL;
        }
        block_list.wilderness->size += size - block_list.wilderness->size;
        stats.num_allocated_bytes += size - block_list.wilderness->size;
        memmove((char*)block_list.wilderness + meta_s, oldp, p_meta->size);
        return (char*)block_list.wilderness + meta_s;
    }
    void* newp = smalloc(size);
    if(newp == nullptr){return NULL;}
    memmove(newp, oldp, p_meta->size);
    sfree(oldp);
    return newp;
}

size_t _num_free_blocks(){ return stats.num_free_blocks; }
size_t _num_free_bytes(){ return stats.num_free_bytes; }
size_t _num_allocated_blocks(){ return stats.num_allocated_blocks; }
size_t _num_allocated_bytes(){ return stats.num_allocated_bytes; }
size_t _num_meta_data_bytes(){ return stats.num_allocated_blocks * meta_s; }
size_t _size_meta_data(){ return meta_s; }