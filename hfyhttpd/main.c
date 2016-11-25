//
//  main.c
//  hfyhttpd
//
//  Created by HFY on 11/15/16.
//  Copyright © 2016 HFY. All rights reserved.
//

#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include "hfyhttpd.h"

struct client_node *clients = NULL;
struct client_node *current_client = NULL;

int main(int argc, const char * argv[])
{
    int servport = HTTPD;
    int httpd = -1;
    
    httpd = start((u_short*)&servport);
    current_client = clients = (struct client_node *)malloc(sizeof(struct client_node));
    
    // 捕获SIG_CHLD信号，处理僵死进程
    signal(SIGCHLD, sig_chld_handler);
    for (; ;) {
        int clifd;
        struct sockaddr_in cliaddr;
        socklen_t cliaddrlen = sizeof(cliaddr);
        pthread_t threadid;
        struct clinfo *client;
        
        client = (struct clinfo *)malloc(sizeof(struct clinfo));
        clifd = accept(httpd, (struct sockaddr *)&cliaddr, &cliaddrlen);
        if (clifd < 0)
        {
            //  子进程终止，父进程捕获到SIGCHLD信号如果不处理，会导致系统调用accept被中断(见UNP 5.9)
            if (errno == EINTR)
                continue;
            else
                perror("accept");
        }
        
        printf("------------------------------------------------\n");
        // 填充client结构
        inet_ntop(AF_INET, &cliaddr.sin_addr, client -> cli_ip, sizeof(client -> cli_ip)); // 填充ip
        client -> cli_port = ntohs(cliaddr.sin_port); // 填充port
        client -> cli_sockfd = clifd; // 填充sockfd
        
        Pthread_create(&threadid, NULL, (void *)service_provider, (void *)client);
        
        client -> cli_threadid = threadid; // 填充threadid
        
        // 保存已连接client到链表，便于管理
        current_client = insert_client(current_client, client);
        printf("client num: %d\n", get_client_count(clients));
        
        //  设置线程为分离状态，线程结束后自动回收
        pthread_detach(threadid);
    }

    return 0;
}
