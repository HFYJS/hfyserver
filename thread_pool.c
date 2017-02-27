//
//  thread_pool.c
//  hfyhttpd
//
//  Created by HFY on 11/26/16.
//  Copyright © 2016 HFY. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "thread_pool.h"

static Thread_pool *pool = NULL; // 全局线程池实例

void init_pool(int thread_size) {
    pool = (Thread_pool *) calloc(1, sizeof(Thread_pool));
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->cond, NULL);
    
    pool->task_header = NULL;
    pool->task_size = 0;
    pool->max_thread_size = thread_size;
    pool->threads = (pthread_t *) calloc(thread_size, sizeof(pthread_t));
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for (int i = 0; i < thread_size; i ++)
        pthread_create(&(pool -> threads[i]), &attr, thread_routine, NULL);
    pthread_attr_destroy(&attr);

    pool->destroy = 0;
}

void join_task(void *(*process)(void *), void *arg) {
    Task *new_task = (Task *) calloc(1, sizeof(Task));
    new_task->process = process;
    new_task->arg = arg;
    new_task->next = NULL;
    
    pthread_mutex_lock(&pool->lock);
    Task *current_task = pool -> task_header;
    if (current_task == NULL)
        pool -> task_header = new_task;
    else {
        while (current_task -> next != NULL)
            current_task = current_task -> next;
        current_task -> next = new_task;
    }
    pool -> task_size++;
    pthread_mutex_unlock(&(pool -> lock));
    
    pthread_cond_signal(&(pool -> cond));
}

void *thread_routine(void *arg) {
    for (; ;)
    {
        pthread_mutex_lock(&(pool -> lock));
        if (pool -> destroy)
        {
            pthread_mutex_unlock(&(pool -> lock));
            pthread_exit(NULL);
        }
        while (!pool -> task_size)
            pthread_cond_wait(&(pool -> cond), &(pool -> lock));
        
        Task *task = pool -> task_header;
        pool -> task_header = task -> next;
        pool -> task_size--;
        pthread_mutex_unlock(&(pool -> lock));
        
        (*(task -> process))(task -> arg);
        
        free((void *)task);
        task = NULL;
    }
    return NULL;
}

void destroy_pool() {
    if (pool == NULL)
        return;
    
    pool -> destroy = 1;
    
    pthread_cond_broadcast(&(pool -> cond));
    for (int i = 0; i < pool -> max_thread_size; i++)
        pthread_join(pool -> threads[i], NULL);
    free((void *)pool -> threads);
    
    Task *current_task = pool -> task_header;
    if (pool -> task_header != NULL)
    {
        while (pool -> task_header -> next != NULL)
        {
            current_task = pool -> task_header;
            pool -> task_header = current_task -> next;
            free((void *)current_task);
        }
        free((void *)pool -> task_header);
    }
    
    pthread_mutex_destroy(&(pool -> lock));
    pthread_cond_destroy(&(pool -> cond));
    
    free((void *)pool);
    pool = NULL;
}