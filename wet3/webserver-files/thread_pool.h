#ifndef __POOL_H__
#define __POOL_H__

#include "segel.h"

typedef int conn_t;
typedef struct thread_pool thread_pool;

typedef struct conn_info {
    conn_t conn;
    struct timeval req_arrival;
} conn_info_t;

typedef struct conn_elem{
    conn_info_t info;
    struct conn_elem *next;
    struct conn_elem *prev;
} conn_elem;

typedef struct conn_queue {
    size_t size;
    conn_elem *head;
    conn_elem *tail;
} conn_queue_t;

conn_queue_t connQueueInit();
void enqueue(conn_queue_t *conn_q, conn_t conn, struct timeval req_arrival);
void dequeue(conn_queue_t *conn_q, conn_info_t* info);
void connQueueDestroy(conn_queue_t *conn_q);

typedef struct thread_args {
    thread_pool *tp;
    int thread_id;
} thread_args_t;

typedef struct thread_stats {
    int thread_id;
    int count;
    int static_count;
    int dynamic_count;
} thread_stats_t;

/**
 * @struct thread pool
 * @brief pool to hold all worker threads in the server.
 * 
 * @param thread_pool::threads - array of worker threads.
 * @param thread_pool::args - array of arguments for worker functions. each arg contains thread_pool and thread worker id.
 * @param thread_pool::buffer_conn - queue for incoming connections.
 * @param thread_pool::handled_conn - if handled_conn[i] = -1 then thread i is idle. else, handled_conn[i] = connfd of handled connection.
 * @param thread_pool::max_threads - max number of threads.
 * @param thread_pool::max_conns - max number of connections.
 * @param thread_pool::num_handled_conn - number of connections currently handled by worker threads.
 * @param thread_pool::conn_lock - mutex.
 * @param thread_pool::conn_cond - condition variable.
 */
typedef struct thread_pool{
    pthread_t *threads;
    thread_stats_t *tstats;
    thread_args_t *args;
    conn_queue_t buffer_conn;
    conn_t *handled_conn;
    size_t max_threads;
    size_t max_conns;
    size_t num_handled_conn;
    pthread_mutex_t conn_lock;
    pthread_cond_t worker_cond;
    pthread_cond_t main_cond;
} thread_pool;

void* tpWorkerHandle(void *args);
thread_pool* threadPoolInit(int max_threads, int max_conns);
void threadPoolDestroy(thread_pool *tp);


typedef struct req_stats{
    conn_t conn;
    struct timeval req_arrival;
    struct timeval req_dispatch;
} req_stats_t;

#endif /* __POOL_H__ */