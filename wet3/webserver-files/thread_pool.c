#include "thread_pool.h"
#include "request.h"

#define IDLE -1

conn_queue_t connQueueInit() {
    conn_queue_t conn_q;
    conn_q.head = conn_q.tail = NULL;
    conn_q.size = 0;
    return conn_q;
}

void enqueue(conn_queue_t *conn_q, conn_t conn) {
    if(conn_q == NULL)
        return;
    if(conn_q->size == 0) {
        if((conn_q->tail = (conn_elem*)malloc(sizeof(conn_elem))) == NULL)
            return;
        conn_q->tail->conn = conn;
        conn_q->tail->next = NULL;
        conn_q->tail->prev = NULL;
        conn_q->head = conn_q->tail;
    }
    else {
        if((conn_q->tail->prev = (conn_elem*)malloc(sizeof(conn_elem))) == NULL)
            return;
        conn_q->tail->prev->conn = conn;
        conn_q->tail->prev->next = conn_q->tail;
        conn_q->tail->prev->prev = NULL;
        conn_q->tail = conn_q->tail->prev;
    }
    conn_q->size++;
}

conn_t dequeue(conn_queue_t *conn_q) {
    conn_t conn;
    if(conn_q == NULL || conn_q->size == 0)
        return -1;
    else if(conn_q->size == 1){
        conn = conn_q->head->conn;
        free(conn_q->head);
        conn_q->head = conn_q->tail = NULL;
    }
    else {
        conn = conn_q->head->conn;
        conn_elem *temp = conn_q->head;
        conn_q->head = conn_q->head->prev;
        free(temp);
        conn_q->head->next = NULL;
    }
    conn_q->size--;
    return conn;
}

void connQueueDestroy(conn_queue_t *conn_q) {
    if(conn_q == NULL)
        return;
    while(conn_q->size > 0) {
        dequeue(conn_q);
    }
    free(conn_q);
}

void* tpWorkerHandle(void *args_v) {
    if(args_v == NULL)
        return NULL;
    thread_args_t *args = (thread_args_t*)(args_v);
    thread_pool *tp = args->tp;
    unsigned int worker_id = args->worker_id;
    while(1) {
        pthread_mutex_lock(&tp->conn_lock);
        while(tp->buffer_conn.size == 0)
            pthread_cond_wait(&tp->conn_cond, &tp->conn_lock);
        // acquired lock and condition holds
        conn_t conn = dequeue(&tp->buffer_conn);
        tp->handled_conn[worker_id] = conn;
        tp->num_handled_conn++;
        pthread_mutex_unlock(&tp->conn_lock);

        requestHandle(conn);
        Close(conn);

        pthread_mutex_lock(&tp->conn_lock);
        tp->num_handled_conn--;
        if(tp->buffer_conn.size + tp->num_handled_conn < tp->max_conns)
            pthread_cond_signal(&tp->conn_cond);
        tp->handled_conn[worker_id] = IDLE;
        pthread_mutex_unlock(&tp->conn_lock);
    }
    return NULL;
}

thread_pool* threadPoolInit(int max_threads, int max_conns)
{
    if(max_threads <= 0 || max_conns <= 0)
        return NULL;
    thread_pool *tp = (thread_pool*)malloc(sizeof(thread_pool));
    if(tp == NULL)
        return NULL;
    if(pthread_mutex_init(&tp->conn_lock, NULL) != 0) {
        threadPoolDestroy(tp);
        return NULL;
    }
    if((tp->threads = (pthread_t*)malloc(sizeof(pthread_t) * max_threads)) == NULL) {
        threadPoolDestroy(tp);
        return NULL;
    }
    tp->buffer_conn = connQueueInit();
    tp->max_threads = max_threads;
    tp->max_conns = max_conns;
    tp->num_handled_conn = 0;
    pthread_cond_init(&tp->conn_cond, NULL);

    if((tp->args = (thread_args_t*)malloc(sizeof(thread_args_t) * max_threads)) == NULL) {
        threadPoolDestroy(tp);
        return NULL;
    }

    if((tp->handled_conn = (conn_t*)malloc(sizeof(conn_t) * max_threads)) == NULL) {
        threadPoolDestroy(tp);
        return NULL;
    }

    for (int i = 0; i < max_threads; i++){
        tp->args[i].tp = tp;
        tp->args[i].worker_id = i;
        tp->handled_conn[i] = IDLE;
		if(pthread_create(&tp->threads[i], NULL, &tpWorkerHandle, &tp->args[i]) != 0) {
            threadPoolDestroy(tp);
            return NULL;
        }
	}

    return tp;
}

void threadPoolDestroy(thread_pool *tp){
    for (size_t i = 0; i < tp->max_threads; i++) {
        if(pthread_join(tp->threads[i], NULL) != 0)
            return;
    }
    free(tp->threads);
    free(tp->args);
    free(tp->handled_conn);
    connQueueDestroy(&tp->buffer_conn);

    if(pthread_cond_destroy(&tp->conn_cond) != 0) {
        free(tp);
        return;
    }

    if(pthread_mutex_destroy(&tp->conn_lock) != 0) {
        free(tp);
        return;
    }

    free(tp);
}