#ifndef SOCKET__H
#define SOCKET__H
#define Buffer_Size 1024*1024
#define FILENAME_H264 "./video/张惠妹-听海-1998.h264"
#define  AAC_FILE_NAME "./video/张惠妹-听海-1998.aac"
#define SLEEP_TIME_H264 1000/H264_MAC_	//1000/H264_MAC_
#define SLEEP_TIME_AAC 1000/AAC_MAC_	//1000/帧数
int createTcpSocket();
int createUdpSocket();
int bindSocket(int sockfd, const char*, int port);
int clientAccept(int sockfd, char* ip, int& port);
void doClient(int clientSockfd, const char* clientIP, int clientPort);
#endif