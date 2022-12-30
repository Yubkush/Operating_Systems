#include "thread_pool.h"
#include "request.h"

void* tpWorkerHandle(void *tp_v) {
    if(tp_v == NULL)
        return NULL;
    thread_pool *tp = (thread_pool*)(tp_v);
    while(1) {
        pthread_mutex_lock(&tp->conn_lock);
        while(tp->conns_q_size == 0)
            pthread_cond_wait(&tp->conn_cond, &tp->conn_lock);
        // acquired lock and condition holds
        conn_t conn = tp->conn_q_head->conn;
        conn_elem *conn_q = tp->conn_q_head;
        tp->conn_q_head = tp->conn_q_head->prev;
        tp->conn_q_head->next = NULL; 
        free(conn_q);
        tp->conns_q_size--;
        if(tp->conns_q_size > 0)
            pthread_cond_signal(&tp->conn_cond);
        pthread_mutex_unlock(&tp->conn_lock);
        requestHandle(conn);
        Close(conn);
    }
    return NULL;
}

thread_pool* threadPoolInit(size_t max_threads, size_t max_conns)
{
    thread_pool *tp = (thread_pool*)malloc(sizeof(thread_pool));
    if(tp == NULL)
        return NULL;
    if(pthread_mutex_init(&tp->conn_lock, NULL) != 0) {
        free(tp);
        return NULL;
    }
    if((tp->threads = (pthread_t*)malloc(sizeof(pthread_t) * max_threads)) == NULL) {
        free(tp);
        return NULL;
    }
    tp->conn_q_head = NULL;
    tp->conn_q_tail = NULL;
    tp->max_threads = max_threads;
    tp->max_conns = max_conns;
    tp->conns_q_size = 0;
    pthread_cond_init(&tp->conn_cond, NULL);

    for (int i = 0; i < max_threads; i++){
		pthread_create(&tp->threads[i], NULL, &tpWorkerHandle, tp);
	}

    return tp;
}

void threadPoolDestroy(thread_pool *tp){
    for (size_t i = 0; i < tp->max_threads; i++) {
        if(pthread_join(tp->threads[i], NULL) != 0)
            return;
    }
    free(tp->threads);

    conn_elem *c_temp = tp->conn_q_tail;
    while(c_temp != NULL) {
        Close(c_temp->conn);
        conn_elem *next = c_temp->next;
        free(c_temp);
        c_temp = next;
    }

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