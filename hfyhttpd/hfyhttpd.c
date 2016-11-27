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
#include <errno.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include "hfyhttpd.h"

int start(u_short *port)
{
    int httpd = -1;
    struct sockaddr_in servaddr;
    int opt_reuse = 1;
    
    //  1.socket
    httpd = Socket(AF_INET, SOCK_STREAM, 0);
    
    //  2.bind
    memset((void*)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(*port);
    //  允许地址重用
    if (setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, &opt_reuse, sizeof(opt_reuse)) < 0) {
        perror("setsockopt");
        exit(1);
    }
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

void *service_provider(void *arg)
{
    // 获取client信息，包含ip, port, fd, threadid
    struct clinfo *client = (struct clinfo *)arg;
    int clifd = client -> cli_sockfd;
    char *clip = client -> cli_ip;
    unsigned short cliport = client -> cli_port;
    printf("a client has connected:ip=%s, port=%d, fd=%d\n", clip, cliport, clifd);
    
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
    
    //  read a line
    len = readline(clifd, buff, sizeof(buff));
    
    //  get the method
    while (!ISSPACE(buff[i]) && (sizeof(method) - 1) > i && i < len)
    {
        method[i] = buff[i];
        i++;
    }
    method[i] = '\0';
    printf("METHOD = %s\n", method);
    
    //  if can't recognize the method, call unimplemented function
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        unimplemented(clifd);
        close(clifd);
        pthread_exit(NULL);
    }
    
    //  discard space
    if (ISSPACE(buff[i]) && i < len)
        i++;
    
    //  get the url
    while (!ISSPACE(buff[i]) && (sizeof(url) - 1) > i && i < len)
        url[j++] = buff[i++];
    url[j] = '\0';
    printf("URL = %s\n", url);
    
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
    
    snprintf(path, sizeof(path), RES_PATH"%s", url);
    if (path[strlen(path) -1] == '/')
        STRCAT(path, "index.html");
    if (stat(path, &st) < 0)
        not_found(clifd);
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
    pthread_exit(NULL);
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
    
    STRCAT(buff, "HTTP/1.1 501 Method Not Implemented\r\n");
   
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

void not_found(int fd)
{
    char buff[BUFFSIZE];
    
    memset(buff, 0, sizeof(buff));
    
    STRCAT(buff, "HTTP/1.1 404 Not Found\r\n");
    
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
    printf("execute_cgi\n");
    ssize_t len;
    char buff[BUFFSIZE];
    int content_length = -1;
    pid_t pid;
    int pipefd[2];
    int pipefd2[2];
    char c;
    
    //  获得Content-Length并丢弃header
    if (!strcasecmp(method, "POST"))
    {
        while ((len = readline(fd, buff, sizeof(buff))) > 0 && strcmp(buff, "\n"))
        {
            buff[15] = '\0';
            if (!strcasecmp(buff, "Content-Length:"))
                content_length = atoi(&buff[16]);
        }
    }
    else if (!strcasecmp(method, "GET"))
    {
        len = readline(fd, buff, sizeof(buff));
        while (len > 0 && strcmp(buff, "\n"))
            readline(fd, buff, sizeof(buff));
    }
    
    //  利用管道与重定向完成父子进程间的通信
    if (pipe(pipefd) < 0)
    {
        execute_failed(fd);
        err_quitthread("pipe");
    }
    if (pipe(pipefd2) < 0)
    {
        execute_failed(fd);
        err_quitthread("pipe");
    }
    if ((pid = fork()) < 0)
    {
        execute_failed(fd);
        err_quitthread("fork");
    }
    if (pid == 0)
    {   //  child process
        close(pipefd[1]);
        close(pipefd2[0]);
        
        dup2(pipefd[0], STDIN_FILENO);
        dup2(pipefd2[1], STDOUT_FILENO);
        
        execl(path, path, (char *)0);
        
        execute_failed(fd);
        err_quitthread("execl");
    }
    else
    {   // father process
        close(pipefd[0]);
        close(pipefd2[1]);
        
        if (!strcasecmp(method, "POST"))
            for (int i = 0; i < content_length; i++)
                if (recv(fd, &c, 1, 0) == 1)
                    write(pipefd[1], &c, 1);
        
        while (read(pipefd2[0], &c,1))
            send(fd, &c, 1, 0);
        
        waitpid(pid, NULL, 0);
        close(pipefd[1]);
        close(pipefd2[0]);
    }
}

void handle_file(int fd, const char *path)
{
    printf("handle_file\n");
    FILE *file;
    
    if ((file = fopen(path, "r")) == NULL) {
        not_found(fd);
        
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
    
    STRCAT(buff, "HTTP/1.1 200 OK\r\n");
    
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

void bad_request(int fd)
{
    char buff[BUFFSIZE];
    
    memset(buff, 0, sizeof(buff));
    
    STRCAT(buff, "HTTP/1.1 400 Bad Request\r\n");
    
    STRCAT(buff, "Content-Type: text/html\r\n");
    STRCAT(buff, SERVER_STRING);
    STRCAT(buff, "\r\n");
    
    STRCAT(buff, "<HTML>\r\n");
    STRCAT(buff, "<HEAD><TITLE>Bad Request</TITLE></HEAD>\r\n");
    STRCAT(buff, "<BODY>\r\n");
    STRCAT(buff, "<P>Your browser sent a bad request, please check it, such as a post without \"Content-Length\".</P>\r\n");
    STRCAT(buff, "</BODY>\r\n");
    STRCAT(buff, "<HTML>\r\n");
    
    if (send(fd, buff, strlen(buff), 0) < 0)
        err_quitthread("send");
}

void execute_failed(int fd)
{
    char buff[BUFFSIZE];
    
    memset(buff, 0, sizeof(buff));
    
    STRCAT(buff, "HTTP/1.1 500 Internal Server Error\r\n");
    
    STRCAT(buff, "Content-Type: text/html\r\n");
    STRCAT(buff, SERVER_STRING);
    STRCAT(buff, "\r\n");
    
    STRCAT(buff, "<HTML>\r\n");
    STRCAT(buff, "<HEAD><TITLE>Internal Server Error</TITLE></HEAD>\r\n");
    STRCAT(buff, "<BODY>\r\n");
    STRCAT(buff, "<P>The Server occurred an error.</P>\r\n");
    STRCAT(buff, "</BODY>\r\n");
    STRCAT(buff, "</HTML>\r\n");
    
    if (send(fd, buff, strlen(buff), 0) < 0)
        err_quitthread("send");
}

void sig_chld_handler(int signo)
{
    pid_t pid;
    int stat;
    
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        printf("child process %d terminated, stat = %d\n", pid, stat);
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

// client链表例程
struct client_node *insert_client(struct client_node *current_client, struct clinfo *client)
{
    struct client_node *temp = (struct client_node *)malloc(sizeof(struct client_node));
    temp -> client = client;
    temp -> next = NULL;
    
    current_client -> next = temp;
    
    return temp;
}
int get_client_count(struct client_node *header)
{
    int count = 0;
    
    while (header -> next != NULL)
    {
        count++;
        header = header -> next;
    }
    
    return count;
}
void list_client(struct client_node *header)
{
    int i = 0;
    
    while (header -> next != NULL)
    {
        i++;
        header = header -> next;
        printf("client %d: ip=%s, port=%d, fd=%d\n", i, header -> client -> cli_ip, header -> client -> cli_port, header -> client -> cli_sockfd);
    }
}
void remove_client(struct client_node *current_client, struct clinfo *client)
{
    while (current_client -> next != NULL)
    {
        struct client_node *temp_client = current_client;
        current_client = current_client -> next;
        if (current_client -> client -> cli_threadid == client -> cli_threadid)
        {
            temp_client -> next = current_client -> next;
            free(current_client);
            free(client);
        }
    }
}
