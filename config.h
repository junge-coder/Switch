#ifndef config_H_
#define config_H_

#define ThreadNum  3
#define MSG_SIZE   4096
#define EPOLL_FILE   100
#define EPOLL_EVENT  100
#define EPOLL_TIME   -1

#define CLINET_R   1
#define CLINET_W   2
#define SERVER_R   3
#define SERVER_W   4
#define LISTEN_CL  5

// 本地端口长短连接，远程服务端口长短连接
#define LOCAL_SHORT_NET    1
#define LOCAL_LONG_NET     2
#define REMOTE_SHORT_NET   3
#define REMOTE_LONG_NET    4

// 报文类型  "trm" 表示终端类型，前2字节值，表示后续报文长度
#define MESSAGE_TYPE  "trm"

#define TIME_OUT    120

#endif
