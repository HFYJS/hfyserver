//
//  thread_pool.h
//  hfyhttpd
//
//  Created by HFY on 11/26/16.
//  Copyright © 2016 HFY. All rights reserved.
//

#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

typedef struct task{
    void *(*process)(void *);
    void *arg;
    
    struct task *next;
} Task;

typedef struct {
    pthread_t *threads;
    int max_thread_size;
    
    Task *task_header;
    int task_size;
    
    pthread_mutex_t lock;
    pthread_cond_t cond;
    
    int destroy;
} Thread_pool;

void init_pool(int);
void join_task(void *(process)(void *), void *);
void destroy_pool();
void *thread_routine(void *);

#endif /* _THREAD_POOL_H_ */
