//
//  thread_pool.h
//  hfyhttpd
//
//  Created by HFY on 11/26/16.
//  Copyright © 2016 HFY. All rights reserved.
//

#ifndef thread_pool_h
#define thread_pool_h

typedef struct task{
    void *(*process)(void *); // 回调函数
    void *arg; // 回调函数的参数
    
    struct task *next; // 索引
} Task;

typedef struct {
    // 线程量
    pthread_t *threads; // 线程池(一块连续内存空间)
     int max_thread_size; // 最大线程数
    
    // 任务量
    Task *task_header; // 任务链表
    int task_size; // 待处理任务数量
    
    // 工具量
    pthread_mutex_t lock;
    pthread_cond_t cond;
    
    // 是否销毁
    int destroy;
} Thread_pool;

void init_pool(int); // 初始化线程池
void join_task(void *(process)(void *), void *); // 抛入线程池任务
void destroy_pool(); // 销毁线程池
void *thread_routine(void *); // 线程处理函数

#endif /* thread_pool_h */
