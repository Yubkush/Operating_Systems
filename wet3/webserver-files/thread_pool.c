#include "thread_pool.h"
#include "request.h"

#define IDLE -1

conn_queue_t connQueueInit() {
    conn_queue_t conn_q;
    conn_q.head = conn_q.tail = NULL;
    conn_q.size = 0;
    return conn_q;
}

void enqueue(conn_queue_t *conn_q, conn_t conn, struct timeval arrival_time) {
    if(conn_q == NULL)
        return;
    if(conn_q->size == 0) {
        if((conn_q->tail = (conn_elem*)malloc(sizeof(conn_elem))) == NULL)
            return;
        conn_q->tail->info = (conn_info_t){conn, arrival_time};
        conn_q->tail->next = NULL;
        conn_q->tail->prev = NULL;
        conn_q->head = conn_q->tail;
    }
    else {
        if((conn_q->tail->prev = (conn_elem*)malloc(sizeof(conn_elem))) == NULL)
            return;
        conn_q->tail->prev->info = (conn_info_t){conn, arrival_time};
        conn_q->tail->prev->next = conn_q->tail;
        conn_q->tail->prev->prev = NULL;
        conn_q->tail = conn_q->tail->prev;
    }
    conn_q->size++;
}

void dequeue(conn_queue_t *conn_q, conn_info_t* info) {
    if(conn_q == NULL || conn_q->size == 0)
        return;
    else if(conn_q->size == 1){
        *info = conn_q->head->info;
        free(conn_q->head);
        conn_q->head = conn_q->tail = NULL;
    }
    else {
        *info = conn_q->head->info;
        conn_elem *temp = conn_q->head;
        conn_q->head = conn_q->head->prev;
        free(temp);
        conn_q->head->next = NULL;
    }
    conn_q->size--;
}

void connQueueDestroy(conn_queue_t *conn_q) {
    if(conn_q == NULL)
        return;
    while(conn_q->size > 0) {
        dequeue(conn_q, NULL);
    }
    free(conn_q);
}

req_stats_t calculateStats(conn_info_t info) {
    struct timeval dispatch, diff;
    //calculate dispatch
    gettimeofday(&dispatch, NULL);
    timersub(&dispatch, &info.req_arrival, &diff);
    return (req_stats_t){info.conn, info.req_arrival, diff};
}

void* tpWorkerHandle(void *args_v) {
    if(args_v == NULL)
        return NULL;
    thread_args_t *args = (thread_args_t*)(args_v);
    thread_pool *tp = args->tp;
    int thread_id = args->thread_id;
    while(1) {
        pthread_mutex_lock(&tp->conn_lock);
        while(tp->buffer_conn.size == 0) {
            // fprintf(stderr, "sleep\n");
            pthread_cond_wait(&tp->worker_cond, &tp->conn_lock);
        }
        // acquired lock and condition holds
        conn_info_t info;
        dequeue(&tp->buffer_conn, &info);
        tp->handled_conn[thread_id] = info.conn;
        tp->num_handled_conn++;
        tp->tstats[thread_id].count++;
        req_stats_t req_stats = calculateStats(info);
        pthread_mutex_unlock(&tp->conn_lock);

        requestHandle(info.conn, &req_stats, &(tp->tstats[thread_id]));
        Close(info.conn);

        pthread_mutex_lock(&tp->conn_lock);
        if(strcmp(tp->sched_alg, "block") == 0 && tp->buffer_conn.size + tp->num_handled_conn == tp->max_conns)
            pthread_cond_signal(&tp->main_cond);
        tp->num_handled_conn--;
        tp->handled_conn[thread_id] = IDLE;
        pthread_mutex_unlock(&tp->conn_lock);
    }
    return NULL;
}

thread_pool* threadPoolInit(int max_threads, int max_conns, char *sched_alg)
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
    if((tp->tstats = (thread_stats_t*)malloc(sizeof(thread_stats_t) * max_threads)) == NULL) {
        threadPoolDestroy(tp);
        return NULL;
    }
    tp->buffer_conn = connQueueInit();
    tp->max_threads = max_threads;
    tp->max_conns = max_conns;
    tp->num_handled_conn = 0;
    tp->sched_alg = sched_alg;
    pthread_cond_init(&tp->worker_cond, NULL);
    pthread_cond_init(&tp->main_cond, NULL);

    if((tp->args = (thread_args_t*)malloc(sizeof(thread_args_t) * max_threads)) == NULL) {
        threadPoolDestroy(tp);
        return NULL;
    }

    if((tp->handled_conn = (conn_t*)malloc(sizeof(conn_t) * max_threads)) == NULL) {
        threadPoolDestroy(tp);
        return NULL;
    }

    for (int i = 0; i < max_threads; i++){
        tp->tstats[i] = (thread_stats_t){i,0,0,0};
        tp->args[i].tp = tp;
        tp->args[i].thread_id = i;
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
    free(tp->tstats);
    connQueueDestroy(&tp->buffer_conn);

    if(pthread_cond_destroy(&tp->worker_cond) != 0) {
        free(tp);
        return;
    }

    if(pthread_cond_destroy(&tp->main_cond) != 0) {
        free(tp);
        return;
    }

    if(pthread_mutex_destroy(&tp->conn_lock) != 0) {
        free(tp);
        return;
    }

    free(tp);
}