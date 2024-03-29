#include "segel.h"
#include "request.h"
#include "thread_pool.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//




// HW3: Parse the new arguments too
void getargs(int *port, int *threads, int *conns, int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <threads>\n", argv[0]);
        exit(1);
    }
    *threads = atoi(argv[2]);
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <queue_size>\n", argv[0]);
        exit(1);
    }
    *conns = atoi(argv[3]);
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <schedalg>\n", argv[0]);
        exit(1);
    }
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, threads, conns, clientlen;
    struct sockaddr_in clientaddr;

    getargs(&port, &threads, &conns, argc, argv);
    char* sched_alg = argv[4];
    // HW3: Create some threads...
    thread_pool *tp = threadPoolInit(threads, conns, sched_alg);
    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        struct timeval arrival;
        if(gettimeofday(&arrival, NULL) != 0){
            return 1;
        }
        // 
        // HW3: In general, don't handle the request in the main thread.
        // Save the relevant info in a buffer and have one of the worker threads 
        // do the work. 
        // 
        pthread_mutex_lock(&tp->conn_lock);
        if(tp->buffer_conn.size + tp->num_handled_conn == tp->max_conns) {
            if(strcmp("dt", sched_alg) == 0) {
                pthread_mutex_unlock(&tp->conn_lock);
                Close(connfd);
                continue;
            }
            else if(strcmp("block", sched_alg) == 0) {
                while(tp->buffer_conn.size + tp->num_handled_conn == tp->max_conns)
                    pthread_cond_wait(&tp->main_cond, &tp->conn_lock);
            }
            else if(strcmp("dh", sched_alg) == 0) {
                conn_info_t conn_dropped;
                enqueue(&tp->buffer_conn, connfd, arrival);
                dequeue(&tp->buffer_conn, &conn_dropped);
                pthread_mutex_unlock(&tp->conn_lock);
                Close(conn_dropped.conn);
                continue;
            }
            else if(strcmp("random", sched_alg) == 0) {
                removeHalfRandom(tp);
                if(tp->buffer_conn.size + tp->num_handled_conn == tp->max_conns){
                    pthread_mutex_unlock(&tp->conn_lock);
                    Close(connfd);
                    continue;
                }
            }
            else {
                return 1;
            }
        }
        enqueue(&tp->buffer_conn, connfd, arrival);
        pthread_cond_signal(&tp->worker_cond);
        pthread_mutex_unlock(&tp->conn_lock);
    }

    threadPoolDestroy(tp);
    return 1;
}


    


 
