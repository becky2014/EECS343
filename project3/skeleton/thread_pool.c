#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>

#include "thread_pool.h"

/**
 *  @struct threadpool_task
 *  @brief the work struct
 *
 *  Feel free to make any modifications you want to the function prototypes and structs
 *
 *  @var function Pointer to the function that will perform the task.
 *  @var argument Argument to be passed to the function.
 */

#define MAX_THREADS 40    //number of worker threads you have    15 different threads   
#define STANDBY_SIZE 10



static void *thread_do_work(void *pool);

/*
 * Create a threadpool, initialize variables, etc
 *
 */
pool_t *pool_create(int num_threads, int queue_size)
{
  // if(num_threads > MAX_THREADS){
  //   num_threads = MAX_THREADS;
  // }
  pool_t *thread_pool = (pool_t *)malloc(sizeof(pool_t));
  //initialize mutex and condition variable
  pthread_mutex_init(&thread_pool->lock, NULL);
  pthread_cond_init (&thread_pool->notify, NULL);
  thread_pool->queue = NULL;  //working queue is empty
  thread_pool->tail = NULL;
  thread_pool->thread_count = num_threads;
  thread_pool->task_queue_size_limit = queue_size;
  thread_pool->shutdown = false;

  int i, rc;
  thread_pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
  for(i=0; i<num_threads; i++){
    // printf("creating thread %d\n", i);
    rc = pthread_create(thread_pool->threads + i, NULL, thread_do_work, (void *)thread_pool);
    if (rc){
         printf("ERROR; return code from pthread_create() is %d\n", rc);
         exit(-1);
      }
  }
    return thread_pool;
}


/*
 * Add a task to the threadpool
 *
 */
int pool_add_task(pool_t *pool, void (*function)(void *), void *argument)
{
  if(pool == NULL || function == NULL || argument == NULL ){
    return -1;
  }
  pthread_mutex_lock(&pool->lock);
  if(pool->task_queue_size_limit == 0){
    pthread_mutex_unlock(&pool->lock);
    return -1;
  }
  pool_task_t *task = (pool_task_t *)malloc(sizeof(pool_task_t));
  task->function = function;
  task->argument = (parse_argument *)argument;
  task->nexttask = NULL;

  //add task into the queue
  if(pool->queue == NULL){
    pool->queue = task;
    pool->tail = task;
  }
  else{
    pool->tail->nexttask = task;
    pool->tail = task;
  }
  pool->task_queue_size_limit--;
  pthread_cond_signal(&pool->notify);
  // printf("Just sent signal.\n");
  pthread_mutex_unlock(&pool->lock);
  return 0;
}



/*
 * Destroy the threadpool, free all memory, destroy treads, etc
 *
 */
int pool_destroy(pool_t *pool)
{
  if(!pool){
    return -1;
  }
  if(pool->threads != NULL){
    //set the boolean shutdown to true
    //wake them up separately one by one, and wait them  thread_join, destory 
    pool->shutdown = true;
    int i;
    //wake up all the threads separately
    for(i=0; i<pool->thread_count; i++){
      pthread_mutex_lock(&pool->lock);
      pthread_cond_signal(&pool->notify);
      pthread_mutex_unlock(&pool->lock);
    }
    //wait all threads to finish
    for(i=0; i<pool->thread_count; i++){
      pthread_join(pool->threads[i], NULL);
    }
    //free queue if queue is not null
    while(pool->queue != NULL){
      pool_task_t * pre = pool->queue;
      pool->task_queue_size_limit++;
      if(pool->queue->nexttask != NULL){
        pool->queue = pool->queue->nexttask;
      } 
      else{
        pool->queue = NULL;
      }
      free(pre->argument);
      free(pre);
    }
    free(pool->threads);
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);
  }
  free(pool);
  return 0;
}



/*
 * Work loop for threads. Should be passed into the pthread_create() method.
 *
 */
static void *thread_do_work(void *pool)     //have all threads sit and wait for work to be assigned to them
{
  pool_t *thread_pool = (pool_t *)pool;

  while(1){
    pthread_mutex_lock(&thread_pool->lock);
    while(!thread_pool->queue){                   //if queue is empty then wait 
      pthread_cond_wait(&thread_pool->notify, &thread_pool->lock);
      //after signal is received and thread is awakened mutex will be automatically locked for use by the thread
      // printf("Condition signal received \n");    
    }
    //if shutdown is true then break
    if(thread_pool->shutdown){
      pthread_mutex_unlock(&thread_pool->lock);
      break;
    }
    //if queue is not empty wake up thread, pull work from the queue and run it
    pool_task_t *task = thread_pool->queue;
    thread_pool->queue = task->nexttask;
    thread_pool->task_queue_size_limit++;
    pthread_mutex_unlock(&thread_pool->lock);      //unlock mutex when the thread is finished with it
    
    (*(task->function))(task->argument);   //run the actual function, run the task
    if(task->function == parseRequest){
      pool_add_task(pool, processRequest, task->argument);
    } 
    else{
      close(((parse_argument *)task->argument)->connfd);
      free(((parse_argument *)task->argument)->request);
      free(((parse_argument *)task->argument));
    }
    free(task);
  }
  pthread_exit(NULL);   //destory threads
  return(NULL);
}
