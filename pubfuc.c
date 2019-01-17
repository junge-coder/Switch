#include "fuction.h"
#include "config.h"

extern sock_key g_Key[EPOLL_FILE];

void initSockArrayData()
{
  int i=0;
  for(;i<EPOLL_FILE;i++)
  {
	memset(g_Key[i].ip,0,sizeof(g_Key[i].ip));
	memset(g_Key[i].key,0,sizeof(g_Key[i].key));
	g_Key[i].flag =0;
	g_Key[i].iLen=0;
	g_Key[i].port=0;
	g_Key[i].sockfd=-1;
	g_Key[i].sockfd2=-1;
	g_Key[i].type=0;
	g_Key[i].timeout =0;
  }

}

void initSockData(sock_key *tmSock)
{
	memset(tmSock->ip,0,sizeof(tmSock->ip));
	memset(tmSock->key,0,sizeof(tmSock->key));
	tmSock->flag =0;
	tmSock->iLen=0;
	tmSock->port=0;
	tmSock->sockfd=-1;
	tmSock->sockfd2=-1;
	tmSock->type=0;
	tmSock->timeout =0;
}

// 添加 连接信息，返回设置地址
void * addSockData(sock_key *tmSock)
{
  int i=0;
  for(;i<EPOLL_FILE;i++)
  {
	if(g_Key[i].type == 0)
	{
	  // 设置sock 信息
	  memcpy(&g_Key[i],tmSock,sizeof(sock_key));
	  break;
	}

	if(i == EPOLL_FILE)
	{
	  return NULL;
	}
  }

  return (void *)&g_Key[i];
}

// Type:0,sockfd 1: sockfd2
void * findSockData(int Sock,int Type)
{
  int i=0;
  for(;i<EPOLL_FILE;i++)
  {
	if(0 == Type && g_Key[i].sockfd == Sock)
	  break;
	else if(1 == Type && g_Key[i].sockfd2 == Sock)
	  break;

	if(i == EPOLL_FILE)
	{
	  return NULL;
	}
  }

  return (void *)&g_Key[i];
}

// 关闭超时链接
void withdrawfd()
{
  int i =0;

  while(1)
  {

    if(0 < g_Key[i].flag)
    {
//	   dcs_log(0,0,"i:%d,flag:%d,time:%ld,now:%ld",i,g_Key[i].flag,g_Key[i].timeout,time(NULL));
       if(g_Key[i].timeout <= time(NULL))
       {
         dcs_log(0,0,"<withdrawfd>i=%d,flag=%d,timeoute!(id_cl:%d-id_sv:%d)",i,g_Key[i].flag,g_Key[i].sockfd,g_Key[i].sockfd2);

         // 客户端 超时
         if(1 == g_Key[i].flag)
         {
           close_socket(g_Key[i].sockfd);
           initSockData(&g_Key[i]);
         }
         // 服务端 超时
         else if(2 == g_Key[i].flag)
         {
           close_socket(g_Key[i].sockfd2);
           initSockData(&g_Key[i]);
         }
       }
    }

    i++;
    if(i == EPOLL_FILE)
    {
      i=0;
      sleep(1);
    }
  }

}



