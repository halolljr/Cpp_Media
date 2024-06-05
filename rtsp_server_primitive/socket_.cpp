#define WIN32_LEAN_AND_MEAN
#include "socket_.h"
#include "do_XXX.h"
#include "rtp.h"
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
int createTcpSocket() {
	SOCKET sockfd;
	int on = 1;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		std::cerr << "socket error..." << std::endl;
		return -1;
	}
	//设置套接字选项，使得地址可以被重用。这样在服务器程序关闭后，如果有其他程序想要使用相同的地址和端口，可以立即使用而不必等待一段时间（TIME_WAIT）
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
	return sockfd;
}

int createUdpSocket() {
	int sockfd;
	int on = 1;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		return -1;
	}
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
	return sockfd;
}

int bindSocket(int sockfd, const char* ip, int port) {
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	//server_addr.sin_addr.S_un.S_addr = inet_addr(ip); inet_addr()将点分十进制转换为网络字节序的二进制形式
	inet_pton(AF_INET, ip, &(server_addr.sin_addr.S_un.S_addr));
	if(bind(sockfd,(struct sockaddr*)(&server_addr),sizeof(struct sockaddr))<0)
		return -1;
	return 0;
}

int clientAccept(int sockfd, char* ip, int& port){
	SOCKET clientfd;
	socklen_t len = 0;
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	len = sizeof(client_addr);

	clientfd = accept(sockfd, (struct sockaddr*)(&client_addr), &len);
	if (clientfd < 0) {
		//std::cerr << "accept error..." << std::endl;
		return -1;
	}
	inet_ntop(AF_INET, &(client_addr.sin_addr), ip, INET_ADDRSTRLEN);
	//inet_ntop(AF_INET,&(client_addr.sin_addr),ip,INET_ADDRSTRLEN);
	port = ntohs(client_addr.sin_port);
	return clientfd;
}

void doClient(int clientSockfd, const char* clientIP, int clientPort) {
	std::string method;
	std::string url;
	std::string version;
	int CSeq = 0;
	//-----------------UDP
	//int serverRtpSockFd = -1, serverRtcpSockFd = -1;	//Rtcp用以传入一些反馈字段，本实例并未使用，课件其非必需性。
	int clientRtpPort = 0, clientRtcpPort = 0;
	char* rBuf = new char[Buffer_Size];
	std::string sBuf;

	while (true) {
		int recvlen = 0;
		recvlen = recv(clientSockfd, rBuf, Buffer_Size-1, 0);
		if (recvlen <= 0) {
			break;
		}
		rBuf[recvlen] = '\0';
		std::string recvStr = rBuf;
		printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		printf("%s rBuf = %s \n", __FUNCTION__, rBuf);
		std::string token;
		std::istringstream ssr(recvStr);

		while (getline(ssr, token, '\n')) {	//解析
			if (token.find("OPTIONS") != std::string::npos ||
				token.find("DESCRIBE") != std::string::npos ||
				token.find("SETUP") != std::string::npos ||
				token.find("PLAY") != std::string::npos) {

				std::istringstream iss(token);
				iss >> method >> url >> version;
				if (iss.fail()) {//截取失败
					//error
				}

			}
			else if (token.find("CSeq") != std::string::npos) {
				if (sscanf_s(token.c_str(), "CSeq: %d\r\n", &CSeq) != 1) {
					//error
				}
			}
			else if (!strncmp(token.c_str(), "Transport:", strlen("Transport:"))) {	 //比较line和"Transport:"这个字符串的前strlen("Transport:")个字符是否相等
				// Transport: RTP/AVP/UDP;unicast;client_port=13358-13359
				// Transport: RTP/AVP;unicast;client_port=13358-13359
				//-----------UDP------------------
				//if (sscanf_s(token.c_str(), "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n", &clientRtpPort, &clientRtcpPort) != 2) {
				//	//error
				//	std::cout << "parse Transport error" << std::endl;
				//}
				if (sscanf_s(token.c_str(), "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n") != 0) {
					// error
					printf("parse Transport error \n");
				}
			}
		}

		if (method == "OPTIONS") {
			if (handlCmd_OPTIONS(sBuf, CSeq)) {
				std::cerr << "falied to handle options..." << std::endl;
				break;
			}
		}
		else if (method == "DESCRIBE") {
			if (handlCmd_DESCIRBE_TCP(sBuf, CSeq, url)) {	//handlCmd_DESCIRBE_UDP(sBuf, CSeq, url)
				std::cerr << "falied to handle descirbe..." << std::endl;
				break;
			}
		}
		else if (method == "SETUP") {
			if (handlCmd_SETUP_TCP(sBuf, CSeq)) {	//handlCmd_SETUP_UDP(sBuf, CSeq, clientRtpPort)
				std::cerr << "failed to handle setup..." << std::endl;
				break;
			}
			//--------------本实例采用TCP传送，因此不需要再特意为RTP或者RTCP建立UDP通道了------------------
			//serverRtpSockFd = createUdpSocket();
			//serverRtcpSockFd = createUdpSocket();
			/*if (serverRtpSockFd < 0 || serverRtcpSockFd < 0) {
				std::cerr << "failed to create udp socket..." << std::endl;
				break;
			}
			if (bindSocket(serverRtpSockFd, "0.0.0.0", SERVER_RTP_PORT) < 0 ||
				bindSocket(serverRtcpSockFd, "0.0.0.0", SERVER_RTCP_PORT) < 0) {
				std::cerr << "failed to bind socket..." << std::endl;
				break;
			}*/
		}
		else if (method == "PLAY") {
			if (handlCmd_PLAY(sBuf, CSeq)) {
				std::cerr << "failed to handle play..." << std::endl;
				break;
			}
		}
		else {
			std::cout << "未定义的method " << method << std::endl;
			break;
		}
		std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
		std::cout << __FUNCTION__ << " " << "sBuf= " << sBuf << std::endl;

		int sendBytes = send(clientSockfd, sBuf.c_str(), sBuf.size(), 0);
		std::cout << "SendBytes: " << sendBytes << std::endl;

		//开始播放，发送RTP包
		if (method == "PLAY") {

			//真正的流媒体服务器要求做到非常的延迟，因此本例(性能极低)是不符合流媒体服务器的要求的
			//-------------传输h264文件------------
			std::thread t1(
				[&]() {
					int frameSize = 0, startCode = 0;
					const int temp_size = 500000;	//如果不够大的话要修改
					char* frame = new char[temp_size];
					//struct RtpPacket* rtpPacket = new struct RtpPacket[temp_size];//长度位为0的柔性数组，new起来很复杂，推荐使用malloc
					struct RtpPacket* rtpPacket = (struct RtpPacket*)malloc(temp_size);
					if (!rtpPacket) {
						std::cerr << "failed to malloc RtpPacket..." << std::endl;
						delete[] frame;
						return;
					}
					FILE* fp;
					fopen_s(&fp, FILENAME_H264, "rb");
					if (!fp) {
						std::cerr << "failed to open " << FILENAME_H264 << std::endl;
						return;
					}
					rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VERSION, RTP_PAYLOAD_TYPE_H264, 0, 0, 0, 0x88923423);	//最后的ssrc是写死的
					std::cout << "start play..." << std::endl;
					std::cout << "client ip is: " << clientIP << std::endl;
					std::cout << "client port is: " << clientPort << std::endl;
					while (true) {
						frameSize = getFrameFromH264File(fp, frame, temp_size);
						if (frameSize < 0) {
							std::cout << "读取" << FILENAME_H264 << "结束，framsize=" << frameSize << std::endl;
							break;
						}
						if (startcode3(frame)) {
							startCode = 3;
						}
						else {
							startCode = 4;
						}
						frameSize -= startCode;
						//------------UDP传送------------
						//rtpSendH264Frame_UDP(serverRtpSockFd, clientIP, clientRtpPort, rtpPacket, frame + startCode, frameSize);
						rtpSendH264Frame_TCP(clientSockfd, rtpPacket, frame + startCode, frameSize);
						Sleep(SLEEP_TIME_H264);	//休眠多少毫秒（如何计算得来？1000 / 视频帧数）->区分（Windows）Sleep[毫秒]，（Linux）unsleep[微秒],（Linux）sleep[秒]的单位级&&如何使用ffmpeg查看修改视频帧数？
						//unsleep();	//1000 / 视频帧数 * 1000(微秒);	
					}
					delete[] frame;	//自动释放存--封装成为一个类
					free(rtpPacket);
				}
			);

			//--------------传输AAC文件-------------------
			std::thread t2(
				[&]() {
					struct AdtHeaders adtsHeader;
					struct RtpPacket* rtpPacket;
					uint8_t* frame;
					const int temp_size_frame = 50000;
					int ret;

					FILE* fp;
					fopen_s(&fp, AAC_FILE_NAME, "rb");
					if (!fp) {
						std::cerr << "读取" << AAC_FILE_NAME << "失败..." << std::endl;
						return;
					}
					frame = new uint8_t[temp_size_frame];
					rtpPacket = (struct RtpPacket*)malloc(temp_size_frame);
					if (!rtpPacket) {
						std::cerr << "failed to malloc RtpPacket..." << std::endl;
						delete[] frame;
						return;
					}
					rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VERSION, RTP_PAYLOAD_TYPE_AAC, 1, 0, 0, 0x32411);

					while (true) {
						static int count = 1;
						ret = fread(frame, 1, 7, fp);	//AAC中AdtHeaders头部占7bytes
						if (ret <= 0 && 1==count) {
							std::cerr << "fread of header aac error..." << std::endl;
							break;
						}
						if (ret < 0) {
							break;
						}
						if (parseAdtHeader(frame, &adtsHeader) < 0) {
							std::cout << "parseAdtsHeader error..." << std::endl;
							break;
						}
						ret = fread(frame, 1, adtsHeader.accFrameLength - 7, fp);	//从AAC中读取减去AdtHeaders头部的数据部分
						if (ret <= 0) {
							std::cout << "fread of payload aac error..." << std::endl;
							break;
						}
						rtpSendAACFrame_TCP(clientSockfd, rtpPacket, frame, adtsHeader.accFrameLength - 7);
						Sleep(SLEEP_TIME_AAC);	//1000/43.06--不存在辅助字段
						count++;
					}
					delete[] frame;
					free(rtpPacket);
				}
			);

			t1.join();
			t2.join();

			break;
		}
		method.clear();
		url.clear();
		CSeq = 0;
	}
	closesocket(clientSockfd);
	//-------------------UDP
	/*if (serverRtpSockFd) {
		closesocket(serverRtpSockFd);
	}
	if (serverRtcpSockFd) {
		closesocket(serverRtcpSockFd);
	}*/

	delete[] rBuf;
}