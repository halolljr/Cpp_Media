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
	//�����׽���ѡ�ʹ�õ�ַ���Ա����á������ڷ���������رպ����������������Ҫʹ����ͬ�ĵ�ַ�Ͷ˿ڣ���������ʹ�ö����صȴ�һ��ʱ�䣨TIME_WAIT��
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
	//server_addr.sin_addr.S_un.S_addr = inet_addr(ip); inet_addr()�����ʮ����ת��Ϊ�����ֽ���Ķ�������ʽ
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
	//int serverRtpSockFd = -1, serverRtcpSockFd = -1;	//Rtcp���Դ���һЩ�����ֶΣ���ʵ����δʹ�ã��μ���Ǳ����ԡ�
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

		while (getline(ssr, token, '\n')) {	//����
			if (token.find("OPTIONS") != std::string::npos ||
				token.find("DESCRIBE") != std::string::npos ||
				token.find("SETUP") != std::string::npos ||
				token.find("PLAY") != std::string::npos) {

				std::istringstream iss(token);
				iss >> method >> url >> version;
				if (iss.fail()) {//��ȡʧ��
					//error
				}

			}
			else if (token.find("CSeq") != std::string::npos) {
				if (sscanf_s(token.c_str(), "CSeq: %d\r\n", &CSeq) != 1) {
					//error
				}
			}
			else if (!strncmp(token.c_str(), "Transport:", strlen("Transport:"))) {	 //�Ƚ�line��"Transport:"����ַ�����ǰstrlen("Transport:")���ַ��Ƿ����
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
			//--------------��ʵ������TCP���ͣ���˲���Ҫ������ΪRTP����RTCP����UDPͨ����------------------
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
			std::cout << "δ�����method " << method << std::endl;
			break;
		}
		std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
		std::cout << __FUNCTION__ << " " << "sBuf= " << sBuf << std::endl;

		int sendBytes = send(clientSockfd, sBuf.c_str(), sBuf.size(), 0);
		std::cout << "SendBytes: " << sendBytes << std::endl;

		//��ʼ���ţ�����RTP��
		if (method == "PLAY") {

			//��������ý�������Ҫ�������ǳ����ӳ٣���˱���(���ܼ���)�ǲ�������ý���������Ҫ���
			//-------------����h264�ļ�------------
			std::thread t1(
				[&]() {
					int frameSize = 0, startCode = 0;
					const int temp_size = 500000;	//���������Ļ�Ҫ�޸�
					char* frame = new char[temp_size];
					//struct RtpPacket* rtpPacket = new struct RtpPacket[temp_size];//����λΪ0���������飬new�����ܸ��ӣ��Ƽ�ʹ��malloc
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
					rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VERSION, RTP_PAYLOAD_TYPE_H264, 0, 0, 0, 0x88923423);	//����ssrc��д����
					std::cout << "start play..." << std::endl;
					std::cout << "client ip is: " << clientIP << std::endl;
					std::cout << "client port is: " << clientPort << std::endl;
					while (true) {
						frameSize = getFrameFromH264File(fp, frame, temp_size);
						if (frameSize < 0) {
							std::cout << "��ȡ" << FILENAME_H264 << "������framsize=" << frameSize << std::endl;
							break;
						}
						if (startcode3(frame)) {
							startCode = 3;
						}
						else {
							startCode = 4;
						}
						frameSize -= startCode;
						//------------UDP����------------
						//rtpSendH264Frame_UDP(serverRtpSockFd, clientIP, clientRtpPort, rtpPacket, frame + startCode, frameSize);
						rtpSendH264Frame_TCP(clientSockfd, rtpPacket, frame + startCode, frameSize);
						Sleep(SLEEP_TIME_H264);	//���߶��ٺ��루��μ��������1000 / ��Ƶ֡����->���֣�Windows��Sleep[����]����Linux��unsleep[΢��],��Linux��sleep[��]�ĵ�λ��&&���ʹ��ffmpeg�鿴�޸���Ƶ֡����
						//unsleep();	//1000 / ��Ƶ֡�� * 1000(΢��);	
					}
					delete[] frame;	//�Զ��ͷŴ�--��װ��Ϊһ����
					free(rtpPacket);
				}
			);

			//--------------����AAC�ļ�-------------------
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
						std::cerr << "��ȡ" << AAC_FILE_NAME << "ʧ��..." << std::endl;
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
						ret = fread(frame, 1, 7, fp);	//AAC��AdtHeadersͷ��ռ7bytes
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
						ret = fread(frame, 1, adtsHeader.accFrameLength - 7, fp);	//��AAC�ж�ȡ��ȥAdtHeadersͷ�������ݲ���
						if (ret <= 0) {
							std::cout << "fread of payload aac error..." << std::endl;
							break;
						}
						rtpSendAACFrame_TCP(clientSockfd, rtpPacket, frame, adtsHeader.accFrameLength - 7);
						Sleep(SLEEP_TIME_AAC);	//1000/43.06--�����ڸ����ֶ�
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