//
//  thread_pool.c
//  hfyhttpd
//
//  Created by HFY on 11/26/16.
//  Copyright © 2016 HFY. All rights reserved.
//

#include <stdlib.h>

#include "thread_pool.h"

static Thread_pool *pool = NULL; // 全局线程池实例

void init_pool(int thread_size)
{
    
}

void join_task(void *(*process)(void *), void *arg)
{
    
}

void destroy_pool()
{
    
}

void *thread_routine(void *arg)
{
    
}
