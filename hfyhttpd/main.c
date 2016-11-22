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
#include "hfyhttpd.h"

int main(int argc, const char * argv[])
{
    int servport = HTTPD;
    int httpd = -1;
    int clifd = -1;
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen = sizeof(cliaddr);
    pthread_t threadid;
    
    httpd = start((u_short*)&servport);
    
    for (; ;) {
        clifd = accept(httpd, (struct sockaddr *)&cliaddr, &cliaddrlen);
        if (clifd < 0)
        {
            perror("accept");
            continue;
        }
        Pthread_create(&threadid, NULL, (void *)service_provider, (void *)&clifd);
    }

    return 0;
}
