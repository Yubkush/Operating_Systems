#ifndef __REQUEST_H__

#include "thread_pool.h"

void requestHandle(int fd, req_stats_t *req_stats, thread_stats_t *thr_stats);

#endif
