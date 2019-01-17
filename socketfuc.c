/*
 * socketfuc.c
 *
 *  Created on: 2016年9月12日
 *      Author: zhouj
 */
#include "fuction.h"
#include "config.h"
extern sock_key g_Key[EPOLL_FILE];

int tcp_open_server(const char *listen_addr,int listen_port)
{
  //description:
  //this function is called by the server end.
  //before listening on socket waiting for requests
  //from clients, the server should create the socket,
  //then bind its port to it.All is done by this function

  //arguments:
  //listen_addr--IP address or host name of the server,maybe NULL
  //listen_port--port# on which the server listen on

  int arg = 1;
  int on;
  int sock;
  struct sockaddr_in sin;
  struct linger linger_str;

  //create the socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock < 0)
    return -1;

  //get the IP address and port#
  memset(&sin, 0, sizeof(struct sockaddr_in));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(listen_port);

  if(listen_addr == NULL)
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
  else
  {
    int addr;
    if((addr = inet_addr(listen_addr)) != INADDR_NONE)
      sin.sin_addr.s_addr = addr;
    else //'listen_addr' may be the host name
    {
      struct hostent *ph;

      ph = gethostbyname(listen_addr);
      if(!ph)
        goto lblError;
      sin.sin_addr.s_addr = ((struct in_addr *)ph->h_addr)->s_addr;
    }
  }

  //set option for socket and bind the (IP,port#) with it
  arg = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&arg, sizeof(arg));
  setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&arg, sizeof(arg));

  if(bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    goto lblError;

  //put the socket into passive open status
  if(listen(sock, 20) < 0)
    goto lblError;
  linger_str.l_onoff = 0;
  linger_str.l_linger = 1;
  setsockopt(sock, SOL_SOCKET, SO_LINGER, &linger_str, sizeof(struct linger));

  on = 1;
  if(setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on)))
    goto lblError;

  return sock;

  lblError: if(sock >= 0)
    close(sock);
  return -1;
}

int tcp_accept_client(int listen_sock,int *client_addr,int *client_port)
{
  struct sockaddr_in cliaddr;
  int addr_len;
  int conn_sock;

  for(;;)
  {
    //try accepting a connection request from clients
    addr_len = sizeof(cliaddr);
    conn_sock = accept(listen_sock, (struct sockaddr *)&cliaddr, &addr_len);

    if(conn_sock >= 0)
      break;
    if(conn_sock < 0 && errno == EINTR)
      continue;
    else
      return -1;
  }

  //bring the client info (IP_address, port#) back to caller
  if(client_addr)
    *client_addr = cliaddr.sin_addr.s_addr;
  if(client_port)
    *client_port = ntohs(cliaddr.sin_port);

  return conn_sock;
}

int tcp_connet_server(char *server_addr,int server_port,int client_port)
{
  //this function is performed by the client end to
  //try to establish connection with server in blocking
  //mode

  int arg,sock,addr;
  struct sockaddr_in sin;
  struct linger linger_str;

  //the address of server must be presented
  if(server_addr == NULL || server_port == 0)
    return -1;

  //create the socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock < 0)
    return -1;

  //set option for socket
  arg = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&arg, sizeof(arg));
  setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&arg, sizeof(arg));
  linger_str.l_onoff = 0;
  linger_str.l_linger = 1;
  setsockopt(sock, SOL_SOCKET, SO_LINGER, &linger_str, sizeof(struct linger));
  struct timeval tv;
  tv.tv_sec = 2;
  tv.tv_usec = 0;

  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

  //if 'client_port' presented,then do a binding
  if(client_port > 0)
  {
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(client_port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    arg = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&arg, sizeof(arg));
    if(0 > bind(sock, (struct sockaddr *)&sin, sizeof(sin)))
      goto lblError;
  }

  //prepare the address of server
  memset(&sin, 0, sizeof(struct sockaddr_in));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(server_port);

  if((addr = inet_addr(server_addr)) != INADDR_NONE)
    sin.sin_addr.s_addr = addr;
  else //'server_addr' may be the host name
  {
    struct hostent *ph;
    ph = gethostbyname(server_addr);
    if(!ph)
      goto lblError;

    sin.sin_addr.s_addr = ((struct in_addr *)ph->h_addr)->s_addr;
  }

  //try to connect to server
  if(connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    goto lblError;
  return sock;

  lblError: if(sock >= 0)
    close(sock);
  return -1;
}

// 终端报文，2字节内容表示长度
int read_msg_from_nac(int conn_sockfd,char *msg_buffer,int nbufsize)
{

  int ret,msg_length,i,l;
  fd_set v_set;
  struct timeval tv;
  ret = read(conn_sockfd, msg_buffer, 2);
  if(ret <= 0) //read error
  {
    if(errno == EINTR || errno == EAGAIN)
      return 0;
    dcs_log(0, 0, "<read_msg_from_nac>recv len fail![%d][%s]", errno, strerror(errno));
    return -1;
  }
  else if(ret < 2)
  {
    dcs_log(msg_buffer, ret, "<read_msg_from_nac--1>recv len error !ret=%d", ret);
    return 0;
  }

  msg_length = ((unsigned char)msg_buffer[0]) * 256 + (unsigned char)msg_buffer[1];

  if(msg_length < 0 || nbufsize < msg_length)
  {

    dcs_log(msg_buffer, 2, "<read_msg_from_nac--2>recv len error!");
    return -1;
  }

  l = tcp_read_nbytes(conn_sockfd,msg_buffer,msg_length);

 /*
  ret = 0;
  l = 0;
  for(i = 0;i < 50;i++)
  {
    FD_ZERO(&v_set);
    tv.tv_sec = 0;
    tv.tv_usec = 50000;
    FD_SET(conn_sockfd, &v_set);
    ret = select(conn_sockfd + 1, &v_set, NULL, NULL, &tv);
    if(ret < 0)
      return 0 - i;
    if(ret == 0)
      continue;
    ret = read(conn_sockfd, msg_buffer + l, msg_length - l);
    if(ret < 0)
    {
      if(errno == EINTR || errno == EAGAIN)
        continue;
      dcs_log(0, 0, "<read_msg_from_nac>recv data fail![%d][%s]", errno, strerror(errno));
      return -1;
    }
    l = l + ret;
    if(msg_length - l <= 0)
      break;
  }
*/
  if(l < msg_length)
    return l;
  return msg_length;
}

// 机构报文，4字节内容表示长度
int read_msg_from_net(int conn_sockfd,char *msg_buffer,int nbufsize)
{
  int ret,msg_length,i,l;
  fd_set v_set;
  struct timeval tv;

  ret = read(conn_sockfd,(char *)msg_buffer,4);
  if(ret < 0) //read error
  {
    if(errno == EINTR || errno == EAGAIN)
      return 0;
    dcs_log(0, 0, "<read_msg_from_net>recv len fail![%d][%s]", errno, strerror(errno));
    return -1;
  }
  else if(ret == 0)
  {
    dcs_log(0, 0, "<read_msg_from_net>socket closed[%d][%s]", errno, strerror(errno));
    return -1;
  }
  else if(ret < 4)
  {
    dcs_log(msg_buffer, ret, "<read_msg_from_net>recv len error !ret=%d", ret);
    return 0;
  }

  msg_length=atoi(msg_buffer);
  if(msg_length <= 0)
  {
//       dcs_log(msg_buffer,4,"<read_msg_from_net>recv len error!");
    return 0;
  }
  if(nbufsize < msg_length)
  {
    dcs_log(msg_buffer, 4, "<read_msg_from_net>recv len error!");
    return 0;
  }

  ret = 0;
  l = 0;
  for(i = 0;i < 50;i++)
  {
    FD_ZERO(&v_set);
    tv.tv_sec = 0;
    tv.tv_usec = 50000;
    FD_SET(conn_sockfd, &v_set);
    ret = select(conn_sockfd + 1, &v_set, NULL, NULL, &tv);
    if(ret < 0)
      return 0 - i;
    if(ret == 0)
      continue;
    ret = read(conn_sockfd, msg_buffer + l, msg_length - l);
    if(ret < 0)
    {
      if(errno == EINTR || errno == EAGAIN)
        continue;
      dcs_log(0, 0, "<read_msg_from_net>recv data fail![%d][%s]", errno, strerror(errno));
      return -1;
    }
    l = l + ret;
    if(msg_length - l <= 0)
      break;
  }
  if(l < msg_length)
    return l;
  return msg_length;
}

// 机构报文，
int read_msg_bnk_len6(int conn_sockfd,char *msg_buffer,int nbufsize)
{
  int ret,msg_length,i,l;
  fd_set v_set;
  struct timeval tv;

  ret = read(conn_sockfd,(char *)msg_buffer,6);
  if(ret < 0) //read error
  {
    if(errno == EINTR || errno == EAGAIN)
      return 0;
    dcs_log(0, 0, "<read_msg_bnk_len>recv len fail![%d][%s]", errno, strerror(errno));
    return -1;
  }
  else if(ret == 0)
  {
    dcs_log(0, 0, "<read_msg_bnk_len>socket closed[%d][%s]", errno, strerror(errno));
    return -1;
  }
  else if(ret < 6)
  {
    dcs_log(msg_buffer, ret, "<read_msg_bnk_len>recv len error !ret=%d", ret);
    return 0;
  }

  msg_length=atoi(msg_buffer);
  if(msg_length <= 0)
  {
//       dcs_log(msg_buffer,4,"<read_msg_from_net>recv len error!");
    return 0;
  }
  if(nbufsize < msg_length)
  {
    dcs_log(msg_buffer, 6, "<read_msg_bnk_len>recv len error!");
    return 0;
  }

  ret = 0;
  l = 0;
  for(i = 0;i < 50;i++)
  {
    FD_ZERO(&v_set);
    tv.tv_sec = 0;
    tv.tv_usec = 50000;
    FD_SET(conn_sockfd, &v_set);
    ret = select(conn_sockfd + 1, &v_set, NULL, NULL, &tv);
    if(ret < 0)
      return 0 - i;
    if(ret == 0)
      continue;
    ret = read(conn_sockfd, msg_buffer + l, msg_length - l);
    if(ret < 0)
    {
      if(errno == EINTR || errno == EAGAIN)
        continue;
      dcs_log(0, 0, "<read_msg_from_net>recv data fail![%d][%s]", errno, strerror(errno));
      return -1;
    }
    l = l + ret;
    if(msg_length - l <= 0)
      break;
  }
  if(l < msg_length)
    return l;
  return msg_length;
}

int read_msg_from_amp(int conn_sockfd,char *msg_buffer, int nbufsize)
{
  //
  // read a message from net. the format of a message
  // is (length_indictor, message_content)

  int  ret, msg_length;

  ret = read(conn_sockfd, (char *)msg_buffer, nbufsize) ;
  if(ret < 0) //read error
  {
    if(errno == EINTR || errno == EAGAIN)
      return 0;
    dcs_log(0, 0, "<read_msg_from_amp>recv len fail![%d][%s]", errno, strerror(errno));
    return -1;
  }
  else if(ret == 0)
  {
    dcs_log(0, 0, "<read_msg_from_amp>socket closed[%d][%s]", errno, strerror(errno));
    return -1;
  }

  return ret;
}

int tcp_write_nbytes(int conn_sock, const void *buffer, int nbytes)
{
  //
  // write "nbytes" bytes to a connected socket
  //

  int  nleft,nwritten;
  char *ptr;

  for(ptr = (char *)buffer, nleft = nbytes; nleft > 0;)
  {
    if ((nwritten = write(conn_sock, ptr, nleft)) <= 0)
    {
      if (errno == EINTR)
          nwritten = 0; //and call write() again
      else
      {
          dcs_log(0,0,"socket write error! errno=%d,errstr=%s",errno,strerror(errno));
          return -1;  //error
      }
    }

    nleft -= nwritten;
    ptr   += nwritten;
  }//for

  return  (nbytes - nleft);
}

int tcp_read_nbytes(int conn_sock, void *buffer, int nbytes)
{
  //
  // read "nbytes" bytes from a connected socket descriptor
  //

  int nleft, nread;
  char *ptr;

  for (ptr = (char *) buffer, nleft = nbytes; nleft > 0;)
  {

    if ((nread = read(conn_sock, ptr, nleft)) < 0)
    {

      if (errno == EINTR)
      {
    	nread = 0; // and call read() again
      }
      else
      {
        dcs_log(0, 0, "<tcp_read_nbytes>socket recv error! errno=%d,errstr=%s", errno, strerror(errno));
        return (-1);
      }
    }

    nleft -= nread;
    ptr += nread;
  } //for

  return (nbytes - nleft);  // return >= 0
}

int write_msg_to_NAC(int conn_sockfd,void *msg_buffer,int nbytes)
{
  //
  // write a message to net. the format of a message
  // is (length_indictor, message_content)
  //

  char buffer[2048 + 4];
  int ret;

  //clear the error number
  errno = -1;
  memset(buffer, 0, sizeof(buffer));

  //first 2 bytes is length indictor
  if(nbytes < 0)
    nbytes = 0;
  buffer[0] = (unsigned char)(nbytes / 256);
  buffer[1] = (unsigned char)(nbytes % 256);
  memcpy(buffer + 2, msg_buffer, nbytes);
  //write the message
  if(nbytes >= 0 && msg_buffer)
  {
    ret = tcp_write_nbytes(conn_sockfd, buffer, nbytes + 2);
    if(ret < (nbytes + 2)) //write error
    {
      dcs_log(0, 0, "<write_msg_to_NAC>send data fail !");
      return -1;
    }
  }

  //return the number of bytes written to socket
  return ret - 2;
}

int write_msg_to_net(int conn_sockfd,void *msg_buffer,int nbytes)
{
  //
  // write a message to net. the format of a message
  // is (length_indictor, message_content)
  //

  char buffer[8192 + 4];
  int ret;

  //clear the error number
  errno = -1;
  memset(buffer, 0, sizeof(buffer));

  //first 4 bytes is length indictor
  if(nbytes < 0)
    nbytes = 0;
  sprintf(buffer, "%04d", nbytes);
  memcpy(buffer + 4, msg_buffer, nbytes);
  //write the message
  if(nbytes >= 0 && msg_buffer)
  {
    ret = tcp_write_nbytes(conn_sockfd, buffer, nbytes + 4);
    if(ret < (nbytes + 4)) //write error
    {
      dcs_log(0, 0, "<write_msg_to_net>send data fail !");
      return -1;
    }
  }

  //return the number of bytes written to socket
  return ret - 4;
}

int write_msg_bnk_len6(int conn_sockfd,void *msg_buffer,int nbytes)
{
  //
  // write a message to net. the format of a message
  // is (length_indictor, message_content)
  //

  char buffer[8192 + 4];
  int ret;

  //clear the error number
  errno = -1;
  memset(buffer, 0, sizeof(buffer));

  //first 4 bytes is length indictor
  if(nbytes < 0)
    nbytes = 0;
  sprintf(buffer, "%06d", nbytes);
  memcpy(buffer + 6, msg_buffer, nbytes);
  //write the message
  if(nbytes >= 0 && msg_buffer)
  {
    ret = tcp_write_nbytes(conn_sockfd, buffer, nbytes + 6);
    if(ret < (nbytes + 6)) //write error
    {
      dcs_log(0, 0, "<write_msg_to_net>send data fail !");
      return -1;
    }
  }

  //return the number of bytes written to socket
  return ret - 6;
}

int write_msg_to_AMP(int conn_sockfd,void *msg_buffer,int nbytes)
{
  //
  // write a message to net. the format of a message
  // is (length_indictor, message_content)
  //

  int ret;

  //clear the error number
  errno = -1;

  ret = tcp_write_nbytes(conn_sockfd, msg_buffer, nbytes);
  if(ret < nbytes) //write error
  {
    dcs_log(0, 0, "<write_msg_to_AMP>send data fail !");
    return -1;
  }
  //return the number of bytes written to socket
  return ret;
}

void setnonblocking(int sock)
{
   int opts;
   opts = fcntl(sock, F_GETFL);
   if(opts < 0)
   {
      perror("fcntl(sock, GETFL)");
      exit(1);
   }
   opts = opts | O_NONBLOCK;
   if(fcntl(sock, F_SETFL, opts) < 0)
   {
      perror("fcntl(sock,SETFL,opts)");
      exit(1);
   }
}

int read_msg(int sock_fd,char *msg_type,char *client_buff,int size)
{
  int rtn=0;

  if(0 == memcmp(msg_type, "trm", 3))
    rtn = read_msg_from_nac(sock_fd, client_buff, size);
  else if(0 == memcmp(msg_type, "bnk", 3))
    rtn = read_msg_from_net(sock_fd, client_buff, size);
  else if(0 == memcmp(msg_type, "amp", 3))
    rtn = read_msg_from_amp(sock_fd, client_buff, size);
  else if(0 == memcmp(msg_type, "bk6", 3))
    rtn = read_msg_bnk_len6(sock_fd, client_buff, size);

  return rtn;
}

int write_msg(int sock_fd,char *msg_type,char *server_buff,int iMsgLen)
{
  int rtn =0;
  if(0 == memcmp(msg_type, "trm", 3))
    rtn = write_msg_to_NAC(sock_fd, server_buff, iMsgLen);
  else if(0 == memcmp(msg_type, "bnk", 3))
    rtn = write_msg_to_net(sock_fd, server_buff, iMsgLen);
  else if(0 == memcmp(msg_type, "amp", 3))
    rtn = write_msg_to_AMP(sock_fd, server_buff, iMsgLen);
  else if(0 == memcmp(msg_type, "bk6", 3))
    rtn = write_msg_bnk_len6(sock_fd, server_buff, iMsgLen);

  return rtn;
}

void close_socket(int fd)
{
  int rtn=-1;
  shutdown(fd, SHUT_RDWR);
  rtn = close(fd);
  dcs_log(0,0,"<close_socket>status:%d,fd:%d",rtn,fd);
}
