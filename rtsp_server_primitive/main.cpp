#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "socket_.h"
#pragma comment(lib,"ws2_32.lib")
#define SERVER_PORT 8554	//更换其他端口wireshark抓包不到rtsp数据，参考文章https://cloud.tencent.com/developer/article/1852545
int main() {
	//启动Windows socket start
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {

		std::cout << "PC Server Socket Start Up Error..." << std::endl;
		return -1;
	}
	//启动Windows socket end
	int serverSockfd;
	serverSockfd = createTcpSocket();
	if (serverSockfd < 0) {
		WSACleanup();
		std::cerr << "falied to create tcp socket..." << std::endl;
		return -1;
	}
	if (bindSocket(serverSockfd, "0.0.0.0", SERVER_PORT)<0) {
		std::cerr << "falied to bind addr..." << std::endl;
		return -1;
	}
	if (listen(serverSockfd, 10) < 0) {
		std::cerr << "listen error..." << std::endl;
		return -1;
	}
	printf("%s rtsp://127.0.0.1:%d \n", __FILE__, SERVER_PORT);
	while (true) {
		int clientSockfd;
		char clientIP[40];
		int clientPort;
		clientSockfd = clientAccept(serverSockfd, clientIP, clientPort);
		if (clientSockfd < 0) {
			std::cerr << "falied to accept client..." << std::endl;
			return -1;
		}
		std::cout << "accept client;client ip: " << clientIP << " ;client Port: " << clientPort << std::endl;
		doClient(clientSockfd, clientIP, clientPort);
	}
	closesocket(serverSockfd);
	WSACleanup();
	return 0;
}