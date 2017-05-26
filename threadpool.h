#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stdio.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct threadpool_t threadpool_t;

threadpool_t *threadpool_init(int nthread,int queue_size);
void threadpool_destroy(threadpool_t *pool);
void threadpool_addtask(threadpool_t *pool,void *(*callback)(void *),void *arg);

#ifdef __cplusplus	
}
#endif

#endif

