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

// ���ض˿ڳ������ӣ�Զ�̷���˿ڳ�������
#define LOCAL_SHORT_NET    1
#define LOCAL_LONG_NET     2
#define REMOTE_SHORT_NET   3
#define REMOTE_LONG_NET    4

// ��������  "trm" ��ʾ�ն����ͣ�ǰ2�ֽ�ֵ����ʾ�������ĳ���
#define MESSAGE_TYPE  "trm"

#define TIME_OUT    120

#endif
