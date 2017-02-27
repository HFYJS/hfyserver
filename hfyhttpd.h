//
//  hfyhttpd.h
//  hfyhttpd
//
//  Created by HFY on 11/15/16.
//  Copyright © 2016 HFY. All rights reserved.
//

#ifndef _HFYHTTPD_H_
#define _HFYHTTPD_H_

#define FONT_NONE "\033[0m"
#define FONT_COLOR_RED "\033[0;31m"

#define SERVER_STRING "SERVER: hfyhttpd/0.1.1\r\n"
#define RES_PATH "/Users/hfy/HFY/hfyserver"
#define HTTPD 8888
#define BACKLOG 5
#define BUFFSIZE 1024
#define METHODSIZE 32
#define ENVSIZE 255
#define MAX_THREAD_SIZE 6

#define LOGSIZE 1024
#define LOG_ENABLE 1

#define ISSPACE(c) isspace((int)c)
#define STRCAT(a, b) strcat(a, b)

struct clinfo {
    char cli_ip[16];
    unsigned short cli_port;
    int cli_sockfd;
    pthread_t cli_threadid;
};

struct client_node {
    struct clinfo *client;
    struct client_node *next;
};

struct client_node *insert_client(struct client_node *, struct clinfo *);

int get_client_count(struct client_node *);

void list_client(struct client_node *);

void remove_client(struct client_node *, struct clinfo *);

int start(u_short *);
void *service_provider(void *);
ssize_t readline(int, char *, size_t);
void unimplemented(int);
void not_found(int);
void execute_cgi(int, const char *, const char *, const char *);
void handle_file(int, const char *);
void header(int);
void cat(int, FILE*);
void bad_request(int);
void execute_failed(int);
void sig_chld_handler(int);

//  wrapper functions
int Socket(int, int, int);
void Bind(int, const struct sockaddr *, socklen_t);
void Getsockname(int, struct sockaddr *, socklen_t *);
void Listen(int, int);
void Pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);

void err_quit(const char *);
void err_quitthread(const char *);
void err_skip(const char *);

void hfylog(const char *, int, ...);

#endif /* _HFYHTTPD_H_ */
