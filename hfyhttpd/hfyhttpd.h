//
//  hfyhttpd.h
//  hfyhttpd
//
//  Created by HFY on 11/15/16.
//  Copyright © 2016 HFY. All rights reserved.
//

#ifndef hfyhttpd_h
#define hfyhttpd_h

#define FONT_NONE "\033[0m"
#define FONT_COLOR_RED "\033[0;31m"

#define SERVER_STRING "SERVER: hfyhttpd/0.1.0\r\n"
#define RES_PATH "/Users/hfy/HFY/hfyserver"
#define HTTPD 8888  //  hfyhttpd port to listen
#define BACKLOG 5   //  max server connections
#define BUFFSIZE 1024   //  buff size to read or write
#define METHODSIZE 32   //  method max size
#define ENVSIZE 255 //  env max size

//  log
#define LOGSIZE 1024    //  Log's max size
#define LOG_ENABLE 1    //  weather the log is usable

//  functions
#define ISSPACE(c) isspace((int)c)
#define STRCAT(a, b) strcat(a, b)

int start(u_short *);
void service_provider(void *);
ssize_t readline(int, char *, size_t);
void unimplemented(int);
void not_found(int);
void execute_cgi(int, const char *, const char *, const char *);
void handle_file(int, const char *);
void header(int);
void cat(int, FILE*);
void bad_request(int);
void execute_failed(int);

//  wrapper functions
int Socket(int, int, int);
void Bind(int, const struct sockaddr *, socklen_t);
void Getsockname(int, struct sockaddr *, socklen_t *);
void Listen(int, int);
int Accept(int, struct sockaddr *, socklen_t *);
void Pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);

//  error functions
void err_quit(const char *);
void err_quitthread(const char *);
void err_skip(const char *);

//  Log functions
void hfylog(const char *, int, ...);

#endif /* hfyhttpd_h */
