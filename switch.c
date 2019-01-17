/*
 ============================================================================
 Name        : switch.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "config.h"
#include "fuction.h"

static pid_t g_pidChild = -1, g_pidParent = -1;
int client_flag = 0,clientFD = 0;
int sever_flag = 0,ServerID = 0;
int ServFlag = 0,epfd = 0;
int linkType = 0,locLinkType = 0;
char msg_type[10],remote_ip[16],msg_prtl[10]={0},http_url[100]={0};
int local_port,remote_port,head_len=0,base_type=0;

sock_key g_Key[EPOLL_FILE+1] = { 0 };

typedef void sigfunc_t(int);
sigfunc_t * catch_signal(int sig_no,sigfunc_t *sig_catcher)
{
  //
  //catch a specified signal with the given handler
  //

  struct sigaction act,oact;

  act.sa_handler = sig_catcher;
  act.sa_flags = 0;

  //block no signals when this signal is being handled.
  sigemptyset(&act.sa_mask);

  if(sig_no == SIGALRM || sig_no == SIGUSR1 || sig_no == SIGUSR2)
  {
#ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT;
#endif
  }
  else
  {
#ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT;
#endif

    //#ifdef SA_RESTART
    // act.sa_flags |=  SA_RESTART;
    //#endif
  }

  if(sigaction(sig_no, &act, &oact) < 0)
    return (sigfunc_t *)0;
  return oact.sa_handler;
}

int catch_all_signals(sigfunc_t *sig_catcher)
{
  int signo;

  for(signo = 1;signo < NSIG;signo++)
  {
    switch(signo)
    {
      case SIGHUP:
      case SIGCONT:
        catch_signal(signo, SIG_IGN);
        break;

      case SIGCHLD:
      default:
        catch_signal(signo, sig_catcher);
        break;
    } //switch
  } //for

  return 0;
}

void switch_exit(int signo)
{
  int i;
  dcs_log(0, 0, "<switch_exit>system  exit! signo=[%d]", signo);
  for(i = 0;i < EPOLL_FILE;i++)
  {

    if(g_Key[i].sockfd > 0)
      close_socket(g_Key[i].sockfd);

    if(g_Key[i].sockfd2 > 0)
      close_socket(g_Key[i].sockfd2);
  }

  kill(0, SIGTERM);
  close(epfd);
  dcs_log_close();
  exit(signo);
}

int main(int argc,char** argv)
{

  char fileName[50] = { 0 };
  int i,temp;

  catch_all_signals(switch_exit);
  signal(SIGPIPE, SIG_IGN);

  memset(msg_type, 0, sizeof(msg_type));
  memset(remote_ip, 0, sizeof(remote_ip));

  for(i = 1;i < argc;i++)
  {

    if(0 == memcmp(argv[i], "remote_link=short", 17))
      linkType = REMOTE_SHORT_NET;
    else if(0 == memcmp(argv[i], "remote_link=long", 16))
      linkType = REMOTE_LONG_NET;

    if(0 == memcmp(argv[i], "local_link=short", 16))
      locLinkType = LOCAL_SHORT_NET;
    else if(0 == memcmp(argv[i], "local_link=long", 15))
      locLinkType = LOCAL_LONG_NET;

    if(0 == memcmp(argv[i], "local_port=", 11))
      local_port = atoi(argv[i] + 11);

    if(0 == memcmp(argv[i], "remote_ip=", 10))
      strcpy(remote_ip, argv[i] + 10);

    if(0 == memcmp(argv[i], "remote_port=", 12))
      remote_port = atoi(argv[i] + 12);

    if(0 == memcmp(argv[i], "head_len=", 9))
      head_len = atoi(argv[i] + 9);

    if(0 == memcmp(argv[i], "base_type=", 10))
      base_type = atoi(argv[i] + 10);

    if(0 == memcmp(argv[i], "msg_type=", 9))
      strcpy(msg_type, argv[i] + 9);

    if(0 == memcmp(argv[i], "msg_prtl=", 9))
      strcpy(msg_prtl, argv[i] + 9);

    if(0 == memcmp(argv[i], "http_url=", 9))
      strcpy(http_url, argv[i] + 9);

    if(0 == memcmp(argv[i], "logfile=", 8))
      strcpy(fileName, argv[i] + 8);
  }

  if(strlen(fileName) < 1)
	sprintf(fileName,"%s.log",argv[0]);
  dcs_log_open(fileName, NULL);

  if(argc <= 1)
  {
    dcs_log(0, 0, "<main>number:%d,param error!", argc);
    goto QUIT_SERVER;
  }

  pthread_t t1;
  dcs_log(0,0,"<main>starting create thread ");

  // 创建线程A
  if(pthread_create(&t1, NULL, (void *)DoLoop, NULL) !=0)
  {
	dcs_log(0,0,"fail to create pthread withdrawfd");
    goto QUIT_SERVER;
  }
  else
	dcs_log(0,0,"<main>create thread DoLoop over ");

  withdrawfd();
//  dcs_log(0,0,"<main>exit process");

  /*  多进程
  dcs_log(0,0,"<main>create Child process");
  if((g_pidChild=fork()) < 0)
	goto QUIT_SERVER;

  if(g_pidChild > 0)
  {
	g_pidParent = getpid();
	withdrawfd();
  }
  else
	DoLoop();
   */



  QUIT_SERVER:
  switch_exit(0);
  return 0;
}

void DoLoop()
{
  int sock_fd,nfds,i,iMsgLen,rtn,flag_cl = 0,flag_sv = 0,iType = 0,
      g_pidChild = 0;
  char client_buff[MSG_SIZE],server_buff[MSG_SIZE],temp_buff[MSG_SIZE],temp[1024];

  struct timeval tv;
  struct epoll_event ev,events[EPOLL_EVENT];

  struct sockaddr_in clientaddr;
  struct sockaddr_in serveraddr;
  struct in_addr in;
  sock_key *spTempKey = NULL,gTmSoc;

  epfd = epoll_create(EPOLL_FILE);

  dcs_log(0, 0, "epoll_create! epfd=%d", epfd);

  while(1)
  {
    // 建立客服端监听
    if(!client_flag)
    {
      sock_fd = tcp_open_server(NULL, local_port);
      if(sock_fd < 0)
      {
        dcs_log(0, 0, "<DoLoop>create client socket erro!");
        sleep(2);
        continue;
      }
      else
      {
        client_flag = 1;
        flag_cl = 0;
        clientFD = sock_fd;
        setnonblocking(clientFD);
        dcs_log(0, 0, "create listen! port:%d clientFD=[%d],client_flag=[%d]", local_port, clientFD, client_flag);
      }
    }

    if(1 == client_flag && 0 == flag_cl)
    {

      initSockData(&gTmSoc);
      gTmSoc.sockfd = clientFD;
      gTmSoc.type = LISTEN_CL;

      // 存储
      memset(&ev, 0, sizeof(ev));
      ev.data.ptr = (void *)addSockData(&gTmSoc);

      //设置要处理的事件类型
      ev.events = EPOLLIN | EPOLLET;

      //注册epoll事件
      rtn = epoll_ctl(epfd, EPOLL_CTL_ADD, clientFD, &ev);
      flag_cl = 1;
      dcs_log(0, 0, "<DoLoop>client socket register,rs=%d", rtn);
    }

    // 建立与后端服务连接 [长连接]
    if(REMOTE_LONG_NET == linkType && 0 == sever_flag)
    {
      sock_fd = tcp_connet_server(remote_ip, remote_port, 0);
      if(sock_fd < 0)
      {
        dcs_log(0, 0, "<DoLoop>connect socket erro! (ip:%s-port:%d)", remote_ip, remote_port);
        sleep(2);
        continue;
      }
      else
      {
        sever_flag = 1;
        flag_sv = 1;
        setnonblocking(sock_fd);
        ServerID = sock_fd;
        dcs_log(0, 0, "connect server! ip:%s port:%d ServerID=[%d],sever_flag=[%d]", remote_ip, remote_port, ServerID, sever_flag);

        memset(&ev, 0, sizeof(ev));
        //设置要处理的事件类型
        ev.events = EPOLLET;

        //注册epoll事件
        rtn = epoll_ctl(epfd, EPOLL_CTL_ADD, ServerID, &ev);
        dcs_log(0, 0, "<DoLoop>server socket register,rs=%d", rtn);
      }
    }

    nfds = epoll_wait(epfd, events, EPOLL_EVENT, EPOLL_TIME);

    //处理所发生的所有事件
    for(i = 0;i < nfds;++i)
    {
      spTempKey = (sock_key *)events[i].data.ptr;
      dcs_log(0, 0, "<for-first>i=%d,nfds=%d,type=%d,cl_id=%d,sv_id=%d", i, nfds, spTempKey->type, spTempKey->sockfd, spTempKey->sockfd2);

      if(spTempKey->type == LISTEN_CL) //如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。
      {

        int iCliAddr,iCliPort;
        sock_fd = tcp_accept_client(clientFD, &iCliAddr, &iCliPort);
        if(sock_fd < 0)
          continue;

        setnonblocking(sock_fd);
        in.s_addr = iCliAddr;

        dcs_log(0, 0, "<Get_client>ip:%s,port:%d,sock_fd=%d", inet_ntoa(in), iCliPort, sock_fd);

        // 设置连接客服端数据
        initSockData(&gTmSoc);
        strcpy(gTmSoc.ip, inet_ntoa(in));
        gTmSoc.port = iCliPort;
        gTmSoc.sockfd = sock_fd;
        gTmSoc.sockfd2 = ServerID;
        gTmSoc.type = CLINET_R;
        if(LOCAL_SHORT_NET == locLinkType)
        {
          // 设置链路超时监听
          gTmSoc.flag = 1;
          gTmSoc.timeout = time(NULL) + TIME_OUT;
        }

        memset(&ev, 0, sizeof(ev));
        // 存储
        ev.data.ptr = (void *)addSockData(&gTmSoc);

        //设置用于注测的读操作事件
        ev.events = EPOLLIN | EPOLLET;
        //注册ev
        epoll_ctl(epfd, EPOLL_CTL_ADD, sock_fd, &ev);

      }
      else if(events[i].events & EPOLLIN) //如果是已经连接的用户，并且收到数据，那么进行读入。
      {
        // 客户端，新连接，且有数据读取
        if(CLINET_R == spTempKey->type)
        {
          sock_fd = spTempKey->sockfd;
          if(sock_fd < 0)
            continue;
        }
        // 服务端，有数据读取
        else if(SERVER_R == spTempKey->type)
        {
          sock_fd = spTempKey->sockfd2;
          if(sock_fd < 0)
          {
            sever_flag = 0;
            dcs_log(0, 0, "<EPOLLIN>server socket exception then reset!");
            continue;
          }
        }
        else
        {
          dcs_log(0, 0, "<EPOLLIN>unknown socket [%d]!",sock_fd);
          continue ;
        }

        memset(client_buff, 0, sizeof(client_buff));
        rtn = read_msg(sock_fd,msg_type,client_buff,MSG_SIZE - 1);
        if(rtn == 0)
          continue;
        // 读取数据失败
        else if(rtn < 0)
        {
          dcs_log(0, 0, "<EPOLLIN-%s>read error ,close socket ! type:[%d], rtn=%d", msg_type,spTempKey->type,rtn);
          close_socket(sock_fd);
          initSockData(spTempKey);
          if(SERVER_R == spTempKey->type)
          {
            if(REMOTE_LONG_NET == linkType)
              sever_flag = 0;
          }

        }
        else
        {
          dcs_log(client_buff, rtn, "<EPOLLIN>type=%d,read message!Len=%d", spTempKey->type, rtn);
          if((head_len > rtn) && (spTempKey->type == CLINET_R))
          {
        	dcs_log(0,0,"<EPOLLIN>,receive length error head_len:[%d]",head_len);
        	close_socket(sock_fd);
        	initSockData(spTempKey);
        	continue;
          }

          iMsgLen = rtn;
          memset(temp_buff,0,sizeof(temp_buff));
          if(0 == strcmp(msg_prtl,"http") && SERVER_R == spTempKey->type)
          {
        	if(strstr(client_buff,"\r\n\r\n"))
        	  strcpy(temp_buff, strstr(client_buff,"\r\n\r\n")+4);
        	else
        	  strcpy(temp_buff, "can't get message");
        	iMsgLen = strlen(temp_buff);
          }
          else
        	memcpy(temp_buff, client_buff, iMsgLen);

          //
          if(CLINET_R == spTempKey->type)
          {
            if(head_len > 0)
            {
              memset(&spTempKey->key, 0, sizeof(spTempKey->key));
              memcpy(&spTempKey->key, client_buff, head_len);
            }

            // 短链接服务 sever_flag
            if(REMOTE_LONG_NET != linkType)
            {
              ServerID = tcp_connet_server(remote_ip, remote_port, 0);
              if(ServerID < 0)
              {
//                sleep(2);
            	if(LOCAL_SHORT_NET == locLinkType)
            	  close_socket(sock_fd);

                dcs_log(0, 0, "<EPOLLIN-short>can't connect server (ip:%s port:%d)!", remote_ip, remote_port);

                // return client error
                memset(temp_buff,0,sizeof(temp_buff));
                memcpy(temp_buff,client_buff,head_len);
                strcpy(temp_buff +head_len,"error!");
                write_msg(sock_fd,msg_type, temp_buff, head_len+6);

                break;
              }

              setnonblocking(ServerID);
              dcs_log(0, 0, "<EPOLLIN-short>connect server! (ip:%s port:%d) ServerID=[%d]", remote_ip, remote_port, ServerID);

              initSockData(&gTmSoc);
              memcpy(&gTmSoc,spTempKey, sizeof(sock_key));
              gTmSoc.flag = 2;
              gTmSoc.sockfd2 = ServerID;
              gTmSoc.type = SERVER_R;
              gTmSoc.timeout = time(NULL) + TIME_OUT;
              dcs_log(0, 0, "<EPOLLIN-short>time=%ld", gTmSoc.timeout);

              memset(&ev, 0, sizeof(ev));
              ev.data.ptr = (void *)addSockData(&gTmSoc);
              //设置要处理的事件类型
              ev.events = EPOLLIN | EPOLLET;
              //注册epoll事件
              rtn = epoll_ctl(epfd, EPOLL_CTL_ADD, ServerID, &ev);
              dcs_log(0, 0, "<EPOLLIN-short>server socket register,rs=%d", rtn);
            }
            sock_fd = ServerID;

            // 转发客户端数据至服务端
            iMsgLen -= head_len;
            memset(server_buff,0,sizeof(server_buff));
            if(1==base_type)
               iMsgLen = Base64_Encode(server_buff,temp_buff+head_len,iMsgLen);
            else
               memcpy(server_buff, temp_buff+head_len, iMsgLen);

             // 报文协议为http
             if(0 == strcmp(msg_prtl,"http"))
             {
			  memset(temp_buff, 0, sizeof(temp_buff));
			  // 联通电子劵路径 xml
			  // sprintf(temp_buff,"POST %s%s HTTP/1.1\r\nHost: %s\r\nContent-Type: text/xml;charset=GBK \r\n\r\n",http_url,server_buff, remote_ip);
			  // 湖南银联AT前置
			  sprintf(temp_buff, "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: text/plain;charset=gbk\r\nContent-length:%d \r\n\r\n%s", http_url, remote_ip, iMsgLen, server_buff);

			  memset(server_buff, 0, sizeof(server_buff));
			  strcpy(server_buff, temp_buff);
			  iMsgLen = strlen(server_buff);
             }
          }
          else if(SERVER_R == spTempKey->type)
          {
        	memset(server_buff,0,sizeof(server_buff));
            memcpy(server_buff, spTempKey->key, head_len);
            if(1==base_type)
              iMsgLen = Base64_Decode(server_buff + head_len,temp_buff,iMsgLen);
            else
              memcpy(server_buff + head_len, temp_buff, iMsgLen);
            iMsgLen += head_len;

            sock_fd = spTempKey->sockfd;
            if(REMOTE_LONG_NET != linkType)
            {
              // add 20181015
              rtn = epoll_ctl(epfd, EPOLL_CTL_DEL, spTempKey->sockfd2, NULL);
              close_socket(spTempKey->sockfd2);
              dcs_log(0,0,"<EPOLLIN>close short_link server sockfd:%d,rtn=%d",spTempKey->sockfd2,rtn);
              // 清除 注册信息
              initSockData(spTempKey);
            }

            // 刷新客户端socketid
            memset(&ev, 0, sizeof(ev));
            ev.data.ptr = (void *)findSockData(sock_fd,0);
            ev.events = EPOLLIN | EPOLLET;
            rtn = epoll_ctl(epfd, EPOLL_CTL_MOD, sock_fd, &ev);
            dcs_log(0,0,"<EPOLLIN>Refresh client sockfd:%d,rtn=%d",sock_fd,rtn);
          }

          rtn = write_msg(sock_fd,msg_type, server_buff, iMsgLen);
          dcs_log(server_buff, iMsgLen, "<EPOLLIN>type=%d,write message!Len=%d,rtn=%d", spTempKey->type, iMsgLen,rtn);
        }
      }
      else if(events[i].events & EPOLLOUT) // 如果有数据发送
      {
        iMsgLen = spTempKey->iLen;
        memset(server_buff, 0, sizeof(server_buff));

        dcs_log(0,0,"<EPOLLOUT> get something ");

      }
    }

  }

  END_SERVER: dcs_log(0, 0, "quit sever!");

}
