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
    char* shched_alg = argv[4];
    // HW3: Create some threads...
    thread_pool *tp = threadPoolInit(threads, conns);

    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

        // 
        // HW3: In general, don't handle the request in the main thread.
        // Save the relevant info in a buffer and have one of the worker threads 
        // do the work. 
        // 
        if(strcmp("dt", shched_alg) == 0) {
            if(tp->buffer_conn.size + tp->num_handled_conn == tp->max_conns) {
                Close(connfd);
                continue;
            }
            pthread_mutex_lock(&tp->conn_lock);
            enqueue(&tp->buffer_conn, connfd);
            pthread_cond_signal(&tp->conn_cond);
            pthread_mutex_unlock(&tp->conn_lock);
        }
        else if(strcmp("block", shched_alg) == 0) {
            pthread_mutex_lock(&tp->conn_lock);
            while(tp->buffer_conn.size + tp->num_handled_conn == tp->max_conns)
                pthread_cond_wait(&tp->conn_cond, &tp->conn_lock);

            // insert conn to back of queue
            enqueue(&tp->buffer_conn, connfd);
            pthread_cond_signal(&tp->conn_cond);
            pthread_mutex_unlock(&tp->conn_lock);
        }
        else if(strcmp("dh", shched_alg) == 0) {
            if(tp->buffer_conn.size + tp->num_handled_conn == tp->max_conns) {
                pthread_mutex_lock(&tp->conn_lock);
                conn_t conn_dropped = dequeue(&tp->buffer_conn);
                enqueue(&tp->buffer_conn, connfd);
                pthread_mutex_unlock(&tp->conn_lock);
                Close(conn_dropped);
                continue;
            }
            pthread_mutex_lock(&tp->conn_lock);
            enqueue(&tp->buffer_conn, connfd);
            pthread_cond_signal(&tp->conn_cond);
            pthread_mutex_unlock(&tp->conn_lock);
        }
        else {
            app_error("bad sched_alg");
            return 1;
        }
    }

    threadPoolDestroy(tp);
    return 1;
}


    


 
