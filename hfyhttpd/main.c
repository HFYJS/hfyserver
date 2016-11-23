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
#include "hfyhttpd.h"

int main(int argc, const char * argv[])
{
    int servport = HTTPD;
    int httpd = -1;
    
    httpd = start((u_short*)&servport);
    
    for (; ;) {
        char cliip[16];
        int clifd;
        struct sockaddr_in cliaddr;
        socklen_t cliaddrlen = sizeof(cliaddr);
        pthread_t threadid;
        
        clifd = accept(httpd, (struct sockaddr *)&cliaddr, &cliaddrlen);
        if (clifd < 0)
        {
            //  子进程终止，父进程捕获到SIGCHLD信号并不处理，导致系统调用accept被中断(见UNP 5.9)
            if (errno == EINTR)
                continue;
            else
                perror("accept");
        }
        inet_ntop(AF_INET, &cliaddr.sin_addr, cliip, sizeof(cliip));
        printf("-------------------------------------\nclient connected:\nIP = %s\nCLIFD = %d\n", cliip, clifd);
        Pthread_create(&threadid, NULL, (void *)service_provider, (void *)&clifd);
        //  释放线程资源
        pthread_detach(threadid);
    }

    return 0;
}
