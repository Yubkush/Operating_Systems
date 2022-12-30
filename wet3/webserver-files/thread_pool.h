#ifndef __POOL_H__
#define __POOL_H__

#include "segel.h"

typedef int conn_t;

typedef struct conn_elem{
    conn_t conn;
    struct conn_elem *next;
    struct conn_elem *prev;
} conn_elem;

typedef struct thread_pool{
    pthread_t *threads;
    conn_elem *conn_q_head;
    conn_elem *conn_q_tail;
    size_t max_threads;
    size_t max_conns;
    size_t conns_q_size;
    pthread_mutex_t conn_lock;
    pthread_cond_t conn_cond;
} thread_pool;

void* tpWorkerHandle(void *tp_v);
thread_pool* threadPoolInit(size_t max_threads, size_t max_conns);
void threadPoolDestroy(thread_pool *tp);

#endif /* __POOL_H__ */