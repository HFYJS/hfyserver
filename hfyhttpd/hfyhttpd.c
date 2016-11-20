//
//  hfyhttpd.c
//  hfyhttpd
//
//  Created by HFY on 11/15/16.
//  Copyright © 2016 HFY. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>
#include "hfyhttpd.h"

int start(u_short *port)
{
    int httpd = -1;
    struct sockaddr_in servaddr;
    
    //  1.socket
    httpd = Socket(AF_INET, SOCK_STREAM, 0);
    
    //  2.bind
    memset((void*)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(*port);
    Bind(httpd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (!*port)
    {
        socklen_t addrlen = sizeof(servaddr);
        Getsockname(httpd, (struct sockaddr*)&servaddr, &addrlen);
    }
    
    //  3.listen
    Listen(httpd, BACKLOG);
    hfylog("HFYHTTPD: server started! port=%d", 1, ntohs(servaddr.sin_port));
    
    return httpd;
}

void service_provider(void *arg)
{
    int clifd = *(int *)arg;
    char buff[BUFFSIZE];
    ssize_t len = 0;
    char method[METHODSIZE];
    char url[BUFFSIZE];
    int i = 0;
    int j = 0;
    int cgi = 0;
    char *query_string = NULL;
    char path[BUFFSIZE];
    struct stat st;
    
    hfylog("-------------------------------------\nHFYHTTPD: clifd %d connected", 1, clifd);
    
    //  read a line
    len = readline(clifd, buff, sizeof(buff));
    printf("FIRST LINE: %s", buff);
    
    //  get the method
    while (!ISSPACE(buff[i]) && (sizeof(method) - 1) > i && i < len)
    {
        method[i] = buff[i];
        i++;
    }
    method[i] = '\0';
    printf("METHOD: %s\n", method);
    
    //  if can't recognize the method, call unimplemented function
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        unimplemented(clifd);
        close(clifd);
        return;
    }
    
    //  discard space
    if (ISSPACE(buff[i]) && i < len)
        i++;
    
    //  get the url
    while (!ISSPACE(buff[i]) && (sizeof(url) - 1) > i && i < len)
        url[j++] = buff[i++];
    url[j] = '\0';
    printf("FULL URL: %s\n", url);
    
    //  handle the url
    if (!strcasecmp(method, "POST"))
        cgi = 1;
    else {
        //  if method is GET, distinguish the base url from the query string
        query_string = url;
        while (*query_string != '?' && *query_string != '\0')
            query_string++;
        if (*query_string == '?')
        {
            cgi = 1;
            *query_string++ = '\0';
        }
    }
    printf("QUERY STRING: %s\n", query_string);
    
    snprintf(path, sizeof(path), RES_PATH"%s", url);
    if (path[strlen(path) -1] == '/')
        STRCAT(path, "index.html");
    printf("PATH: %s\n", path);
    if (stat(path, &st) < 0)
    {
//        while (len > 0 && strcmp(buff, "\r\n")) {
//            len = readline(clifd, buff, sizeof(buff));
//            printf("DISCARD BUFF: len=%ld, content=%s", len, buff);
//        }
        notfound(clifd);
    }
    else
    {
        if ((st.st_mode & S_IFMT) == S_IFDIR)
            STRCAT(path, "/index.html");
        if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
            cgi = 1;
        if (cgi)
            execute_cgi(clifd, path, method, query_string);
        else
            handle_file(clifd, path);
    }
    
    close(clifd);
}

ssize_t readline(int fd, char *buff, size_t buffsize)
{
    char c = '\0';
    ssize_t len = 0;
    
    while ((buffsize - len) > 1 && c != '\n')
    {
        if (recv(fd, &c, 1, 0) == 1)
        {
            if (c == '\r')
            {
                if (recv(fd, &c, 1, MSG_PEEK) == 1)
                {
                    if (c == '\n')
                        recv(fd, &c, 1, 0);
                    else
                        c = '\n';
                }
            }
            *buff++ = c;
            len++;
        }
    }
    *buff = '\0';
    
    return len;
}

void unimplemented(int fd)
{
    char buff[BUFFSIZE];
    
    memset(buff, 0, sizeof(buff));
    
    STRCAT(buff, "HTTP/1.0 501 Method Not Implemented\r\n");
   
    STRCAT(buff, "Content-Type: text/html\r\n");
    STRCAT(buff, SERVER_STRING);
    
    STRCAT(buff, "/r/n");
    STRCAT(buff, "<HTML>\r\n");
    STRCAT(buff, "<HEAD><TITLE>Method Not Implemented</TITLE></HEAD>\r\n");
    STRCAT(buff, "<BODY><P>HTTP request not supported.</P></BODY>\r\n");
    STRCAT(buff, "</HTML>\r\n");
    
    if (send(fd, buff, strlen(buff), 0) < 0)
        err_quitthread("send");
}

void notfound(int fd)
{
    char buff[BUFFSIZE];
    
    memset(buff, 0, sizeof(buff));
    
    STRCAT(buff, "HTTP/1.0 404 Not Found\r\n");
    
    STRCAT(buff, "Content-Type: text/html\r\n");
    STRCAT(buff, SERVER_STRING);
    STRCAT(buff, "\r\n");
    
    STRCAT(buff, "<HTML>\r\n");
    STRCAT(buff, "<HEAD><TITLE>Not Found</TITLE></HEAD>\r\n");
    STRCAT(buff, "<BODY>\r\n");
    STRCAT(buff, "<P>The server could not fulfill your request,</P>\r\n");
    STRCAT(buff, "<P>because the resource specifield is unavailable or nonexistent.</P>\r\n");
    STRCAT(buff, "</BODY>\r\n");
    STRCAT(buff, "</HTML>\r\n");
    
    if (send(fd, buff, strlen(buff), 0) < 0)
        err_quitthread("send");
}

void execute_cgi(int fd, const char *path, const char *method, const char *params)
{
    printf("EXCUTE_CGI: path=%s\nmethod=%12s\nparams=%12s\n", path, method, params);
}

void handle_file(int fd, const char *path)
{
    printf("HANDLE_FILE: path=%s\n", path);
    
    FILE *file;
    
    if ((file = fopen(path, "r")) == NULL) {
        notfound(fd);
        
        err_quitthread("fopen");
    } else {
        header(fd);
        cat(fd, file);
        
        fclose(file);
    }
}

void header(int fd)
{
    char buff[BUFFSIZE];
    
    memset(buff, 0, sizeof(buff));
    
    STRCAT(buff, "HTTP/1.0 200 OK\r\n");
    
    STRCAT(buff, "Content-Type: text/html\r\n");
    STRCAT(buff, SERVER_STRING);
    STRCAT(buff, "\r\n");
    
    if (send(fd, buff, strlen(buff), 0) < 0)
        err_quitthread("send");
}

void cat(int fd, FILE *file)
{
    char buff[BUFFSIZE];
    
    while (fgets(buff, sizeof(buff), file) != NULL)
        if (send(fd, buff, strlen(buff), 0) < 0) {
            fclose(file);
            err_quitthread("send");
        }
    if (ferror(file)) {
        fclose(file);
        err_quitthread("fgets");
    }
}

//  wrapper functions implementation
int Socket(int domain, int type, int protocal)
{
    int fd = -1;
    
    if ((fd = socket(domain, type, protocal)) < 0)
        err_quit("socket");
    
    return fd;
}

void Bind(int sockfd, const struct sockaddr *sockaddr, socklen_t addrlen)
{
    if (bind(sockfd, sockaddr, addrlen) < 0)
        err_quit("bind");
}

void Getsockname(int sockfd, struct sockaddr *sockaddr, socklen_t *addrlen)
{
    if (getsockname(sockfd, sockaddr, addrlen) < 0)
        err_quit("getsockname");
}

void Listen(int sockfd, int backlog)
{
    if (listen(sockfd, backlog) < 0)
        err_quit("listen");
}

int Accept(int sockfd, struct sockaddr *sockaddr, socklen_t *addrlen)
{
    int clifd = -1;
    
    if ((clifd = accept(sockfd, sockaddr, addrlen)) < 0)
        err_skip("accept");
    
    return clifd;
}

void Pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
    if (pthread_create(thread, attr, start_routine, arg) != 0)
        err_skip("pthread_create");
}

//  error functions implementation
void err_quit(const char *info)
{
    perror(info);
    exit(1);
}

void err_quitthread(const char *info)
{
    perror(info);
    exit(1);
}

void err_skip(const char *info)
{
    perror(info);
    exit(1);
}

//  log functions
void hfylog(const char *info, int count, ...)
{
    if (!LOG_ENABLE)
        return;
    
    char logstr[LOGSIZE];
    
    if (count)
    {
        va_list args;
        va_start(args, count);
        for (int i = 0; i < 1; i++)
        {
            int arg = va_arg(args, int);
            snprintf(logstr, sizeof(logstr), info, arg);
        }
        va_end(args);
    }
    else
        snprintf(logstr, sizeof(logstr), "%s", info);
    
    fputs(logstr, stderr);
    fputs("\n", stderr);
}
