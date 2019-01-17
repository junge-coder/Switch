/*
 * fuction.h
 *
 *  Created on: 2016年9月12日
 *      Author: zhouj
 */

#ifndef FUCTION_H_
#define FUCTION_H_
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>  /* basic socket definitions */
#include <sys/time.h>    /* timeval{} for select() */
#include <time.h>        /* timespec{} for pselect() */
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>   /* inet(3) functions */
#include <errno.h>
#include <fcntl.h>       /* for nonblocking */
#include <netdb.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <stddef.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <sys/epoll.h>

typedef struct
{

  char buffer[2048];
  int length;
  int sock;
}param;

typedef struct
{
  int sockfd;
  time_t timeout;
  int port;
  int sockfd2;
  char ip[16];
  char type;
  // 超时标志 1、客户端为短链接 2、服务端为短链接
  char flag;
  char key[40];
  int  iLen;
}sock_key;

int tcp_open_server(const char *listen_addr,int listen_port);
int tcp_connet_server(char *server_addr,int server_port,int client_port);
int tcp_accept_client(int listen_sock,int *client_addr,int *client_port);
int read_msg_from_nac(int conn_sockfd,char *msg_buffer,int nbufsize);
int read_msg_from_net(int conn_sockfd,char *msg_buffer,int nbufsize);
int read_msg_from_amp(int conn_sockfd,char *msg_buffer,int nbufsize);
int tcp_write_nbytes(int conn_sock, const void *buffer, int nbytes);
int write_msg_to_NAC(int conn_sockfd,void *msg_buffer,int nbytes);
int write_msg_to_net(int conn_sockfd,void *msg_buffer,int nbytes);
int write_msg_to_AMP(int conn_sockfd,void *msg_buffer,int nbytes);
void setnonblocking(int sock);
int read_msg(int sock_fd,char *msg_type,char *client_buff,int size);
int write_msg(int sock_fd,char *msg_type,char *server_buff,int iMsgLen);

void withdrawfd();
void DoLoop();

void threadsend(void *arg);
void thread_create(void);

void thread_wait(void);
#endif /* FUCTION_H_ */
