#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include "util.h"



typedef struct pool_task_t {
    void (*function)(void *);
    void *argument;
    // parse_argument *argument;
    struct pool_task_t *nexttask;
} pool_task_t;

typedef struct {
  pthread_mutex_t lock;     //lock of working queue  ??? or associated mutex
  pthread_cond_t notify;    //condition variables
  pthread_t *threads;       //pointer to threads
  pool_task_t *queue;       //head pointer of working queue
  pool_task_t *tail;        //tail pointer of working queue
  int thread_count;         //current number of avaliable threads in the pool
  int task_queue_size_limit; //if > queue size, no more request accepted
  bool shutdown;             //shutdown boolean 
} pool_t;



pool_t *pool_create(int thread_count, int queue_size);

int pool_add_task(pool_t *pool, void (*routine)(void *), void *arg);

int pool_destroy(pool_t *pool);

#endif
