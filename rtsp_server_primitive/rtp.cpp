#include "rtp.h"
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
void rtpHeaderInit(struct RtpPacket* rtpPacket, uint8_t csrLen, uint8_t extension, 
	uint8_t padding, uint8_t version, uint8_t payloadTYpe, 
	uint8_t marker, uint16_t seq, uint32_t timestamp, uint32_t ssrc) {

	rtpPacket->rtpHeader.csrcLen = csrLen;
	rtpPacket->rtpHeader.extension = extension;
	rtpPacket->rtpHeader.padding = padding;
	rtpPacket->rtpHeader.version = version;
	rtpPacket->rtpHeader.payloadType = payloadTYpe;
	rtpPacket->rtpHeader.marker = marker;
	rtpPacket->rtpHeader.seq = seq;
	rtpPacket->rtpHeader.timestamp = timestamp;
	rtpPacket->rtpHeader.ssrc = ssrc;
}

int rtpSendPacketOverTcp(int clientSockFd, struct RtpPacket* rtpPacket, uint32_t datasize,char channel) {
	rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);	//uint16_t :unsigned short
	rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);	//uint32_t :unsigned long
	rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);

	uint32_t rtpSize = RTP_HEADER_SIZE+datasize;
	//注意要添加多的四个字节
	char* temBuf = new char[4 + rtpSize];
	temBuf[0] = 0x24;	//$
	temBuf[1] = channel;
	temBuf[2] = (uint8_t)(((rtpSize) & 0xFF00) >> 8);
	temBuf[3] = (uint8_t)((rtpSize) & 0xFF);
	memcpy(temBuf + 4, (char*)(rtpPacket), rtpSize);

	int ret = send(clientSockFd, temBuf, 4 + rtpSize, 0);

	rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
	rtpPacket->rtpHeader.timestamp = ntohl(rtpPacket->rtpHeader.timestamp);
	rtpPacket->rtpHeader.ssrc = ntohl(rtpPacket->rtpHeader.ssrc);

	delete[] temBuf;
	temBuf = nullptr;
	
	return ret;
}

int rtpSendPacketOverUdp(int serverSockFd, const char* ip, uint16_t port, struct RtpPacket* rtpPacket, uint32_t datasize) {
	struct sockaddr_in addr;
	int ret;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	//addr.sin_addr.S_un.S_addr = inet_addr(ip);不安全函数
	inet_pton(AF_INET, ip, &(addr.sin_addr.S_un.S_addr));	//将点分十进制转化为网络字节序
	
	rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
	rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);
	rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);

	ret = sendto(serverSockFd, (char*)rtpPacket, datasize + RTP_HEADER_SIZE, 0, (struct sockaddr*)&addr, sizeof(addr));

	rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
	rtpPacket->rtpHeader.timestamp = ntohl(rtpPacket->rtpHeader.timestamp);
	rtpPacket->rtpHeader.ssrc = ntohl(rtpPacket->rtpHeader.ssrc);

	return ret;
}

int getFrameFromH264File(FILE* fp, char* frame, int size) {
	//读取数据这一段有点难
	//建议结合NAUL来分析这里的文件移动和读取数据！！！
	int rSize, frameSize;
	char* nextStartCode;
	if (!fp) {
		return -1;
	}

	rSize = fread(frame, 1, size, fp);//读取size个字节数为1的数据块到frame中，并且返回实际读到的字节数
	if (!startcode3(frame) && !startcode4(frame)) {
		return -1;
	}
	nextStartCode = findNextStartCode(frame + 3, rSize - 3);	//千万别释放nextSatrtCode,因为此时共同管理内存。
	if (!nextStartCode) {
		return -1;
	}
	else {
		frameSize = (nextStartCode - frame);
		fseek(fp, frameSize - rSize, SEEK_CUR);//这里文件指针的偏移量为负值，所以会让文件（注意是文件指针）从当前位置向前偏移
	}

	return frameSize;
}

inline int startcode3(char* buf) {
	if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1) {
		return 1;
	}
	else
		return 0;
}
inline int startcode4(char* buf) {
	if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1) {
		return 1;
	}
	else
		return 0;
}
char* findNextStartCode(char* buf, int len) {
	//不懂为何要传进来时候rSize-3，然后循环又len-3.
	int i;
	if (len < 3) {
		return nullptr;
	}
	for ( i = 0; i < len - 3; ++i) {
		if (startcode3(buf) || startcode4(buf)) {
			return buf;
		}
		++buf;
	}
	//??
	if (startcode3(buf)) {
		return buf;
	}
	return nullptr;
}

int rtpSendH264Frame_UDP(int serverRtpSockFd, const char* ip, int16_t port, 
			struct RtpPacket* rtpPacket, char* frame, uint32_t frameSize) {

	uint8_t naluType;	//NALU第一个字节
	int sendBytes = 0;
	int ret = 0;

	naluType = frame[0];

	//std::cout << "framsSize=" << frameSize << std::endl;

	if (frameSize <= RTP_MAX_PKT_SIZE) {	//NALU长度小于最大包长，单一NALU模式

		//*   0 1 2 3 4 5 6 7 8 9
		 //*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 //*  |F|NRI|  Type   | a single NAL unit ... |
		 //*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		memcpy(rtpPacket->payload, frame, frameSize);
		//-----------UDP传送
		ret = rtpSendPacketOverUdp(serverRtpSockFd, ip, port, rtpPacket, frameSize);
		if (ret < 0) {
			return - 1;
		}
		rtpPacket->rtpHeader.seq++;
		sendBytes += ret;
		if ((naluType & 0x1F)==6||(naluType & 0x1F) == 7 || (naluType & 0x1F) == 8) {	//如果是SEI,SPS，PPS，就不需要加载时间戳
			goto out;
		}
	}
	else {	//NALU长度大于最大包长，分片模式
		 
		//*  0                   1                   2
		 //*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
		 //* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 //* | FU indicator  |   FU header   |   FU payload   ...  |
		 //* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+



		 //*     FU Indicator
		 //*    0 1 2 3 4 5 6 7
		 //*   +-+-+-+-+-+-+-+-+
		 //*   |F|NRI|  Type   |
		 //*   +---------------+



		 //*      FU Header
		 //*    0 1 2 3 4 5 6 7
		 //*   +-+-+-+-+-+-+-+-+
		 //*   |S|E|R|  Type   |
		 //*   +---------------+

		int pktNum = frameSize/RTP_MAX_PKT_SIZE;	//有几个剩余的包
		int remainPktSize = frameSize % RTP_MAX_PKT_SIZE;	//剩余不完整的包的大小
		int i=0, pos = 1;

		//发送完整的包
		for (int i = 0; i < pktNum; ++i) {
			rtpPacket->payload[0] = (naluType & 0x60) | 28;	//(naluType & 0x60)按位与运算，保留高三位；|28，按位与运算，设置结果的低五位，指定 FU-A 类型，符合 H.264 规范
			rtpPacket->payload[1] = naluType & 0x1F;	//提取 NALU 类型的低五位
			if (0 == i) {	//第一包数据
				rtpPacket->payload[1] |= 0x80;	//start
			}
			else if (0 == remainPktSize && pktNum - 1 == i) {	//最后一包数据
				rtpPacket->payload[1] |= 0x40;	//end
			}
			memcpy(rtpPacket->payload + 2, frame + pos, RTP_MAX_PKT_SIZE);
			ret = rtpSendPacketOverUdp(serverRtpSockFd, ip, port, rtpPacket, RTP_MAX_PKT_SIZE + 2);
			if (ret < 0) {
				return -1;
			}
			rtpPacket->rtpHeader.seq++;
			sendBytes += ret;
			pos += RTP_MAX_PKT_SIZE;
		}
		//发送剩余的数据
		if (remainPktSize > 0) {
			rtpPacket->payload[0] = (naluType & 0x60) | 28;
			rtpPacket->payload[1] = naluType & 0x1F;
			rtpPacket->payload[1] |= 0x40;	//end

			memcpy(rtpPacket->payload + 2, frame + pos, remainPktSize + 2);
			ret = rtpSendPacketOverUdp(serverRtpSockFd, ip, port, rtpPacket, remainPktSize + 2);
			if (ret < 0) {
				return -1;
			}
			rtpPacket->rtpHeader.seq++;
			sendBytes += ret;
		}
	}
	rtpPacket->rtpHeader.timestamp += 90000 / H264_MAC_;	//时钟周期/视频帧数
	//时钟周期：这里将一秒定义为90000毫秒（h264时间基）
	//请先检查你的视频帧数
out:
	return sendBytes;
}

int rtpSendH264Frame_TCP(int clientSockFd, struct RtpPacket* rtpPacket, char* frame, const uint32_t& frameSize) {
	uint8_t naluType;	//NALU第一个字节
	int sendBytes = 0;
	int ret = 0;

	naluType = frame[0];

	//std::cout << __FUNCTION__<<"---framsSize=" << frameSize << std::endl;

	if (frameSize <= RTP_MAX_PKT_SIZE) {	//NALU长度小于最大包长，单一NALU模式

		//*   0 1 2 3 4 5 6 7 8 9
		 //*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 //*  |F|NRI|  Type   | a single NAL unit ... |
		 //*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

		memcpy(rtpPacket->payload, frame, frameSize);
		ret = rtpSendPacketOverTcp(clientSockFd,rtpPacket,frameSize,0x00);	//0x00 总共有两路--视频流与音频流，视频流RTP和RTCP占两个通道（0&1），音频流占两个通道（2&3），因此从0开头
		if (ret < 0) {
			return -1;
		}
		rtpPacket->rtpHeader.seq++;
		sendBytes += ret;
		if ((naluType & 0x1F) == 6 || (naluType & 0x1F) == 7 || (naluType & 0x1F) == 8) {	//如果是SEI,SPS，PPS，就不需要加载时间戳
			goto out;
		}
	}
	else {	// nalu长度小于最大包：分片模式
		 
		 //*  0                   1                   2
		 //*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
		 //* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 //* | FU indicator  |   FU header   |   FU payload   ...  |
		 //* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+



		 //*     FU Indicator
		 //*    0 1 2 3 4 5 6 7
		 //*   +-+-+-+-+-+-+-+-+
		 //*   |F|NRI|  Type   |
		 //*   +---------------+



		 //*      FU Header
		 //*    0 1 2 3 4 5 6 7
		 //*   +-+-+-+-+-+-+-+-+
		 //*   |S|E|R|  Type   |
		 //*   +---------------+

		int pktNum = frameSize / RTP_MAX_PKT_SIZE;	//有几个剩余的包
		int remainPktSize = frameSize % RTP_MAX_PKT_SIZE;	//剩余不完整的包的大小
		int i = 0, pos = 1;

		//发送完整的包
		for (i = 0; i < pktNum; ++i) {
			rtpPacket->payload[0] = (naluType & 0x60) | 28;	//(naluType & 0x60)按位与运算，保留高三位；|28，按位与运算，设置结果的低五位，指定 FU-A 类型，符合 H.264 规范
			rtpPacket->payload[1] = naluType & 0x1F;	//提取 NALU 类型的低五位
			if (0 == i) {	//第一包数据
				rtpPacket->payload[1] |= 0x80;	//start
			}
			else if (0 == remainPktSize && pktNum - 1 == i) {	//最后一包数据
				rtpPacket->payload[1] |= 0x40;	//end
			}
			memcpy(rtpPacket->payload + 2, frame + pos, RTP_MAX_PKT_SIZE);
			ret = rtpSendPacketOverTcp(clientSockFd, rtpPacket, RTP_MAX_PKT_SIZE+2, 0x00);	//0x00 总共有两路--视频流与音频流，视频流RTP和RTCP占两个通道（0&1），音频流占两个通道（2&3），因此从0开头
			if (ret < 0) {
				return -1;
			}
			rtpPacket->rtpHeader.seq++;
			sendBytes += ret;
			pos += RTP_MAX_PKT_SIZE;
		}
		//发送剩余的数据
		if (remainPktSize > 0) {
			rtpPacket->payload[0] = (naluType & 0x60) | 28;
			rtpPacket->payload[1] = naluType & 0x1F;
			rtpPacket->payload[1] |= 0x40;	//end

			memcpy(rtpPacket->payload + 2, frame + pos, remainPktSize + 2);
			ret = rtpSendPacketOverTcp(clientSockFd, rtpPacket, remainPktSize + 2, 0x00);
			if (ret < 0) {
				return -1;
			}
			rtpPacket->rtpHeader.seq++;
			sendBytes += ret;
		}
	}
	rtpPacket->rtpHeader.timestamp += H264_TIMESTAMP_;	//时钟周期/视频帧数
	//时钟周期：这里将一秒定义为90000毫秒（h264时间基）
	//请先检查你的视频帧数
out:
	return sendBytes;
}

int parseAdtHeader(uint8_t* in, struct AdtHeaders* res) {

	static int frame_number = 0;
	memset(res, 0, sizeof(*res));

	if ((in[0] == 0xFF) && ((in[1] & 0xF0) == 0xF0)) {//in[0]八个bit（uint8_t）加 in[1]&0xF0 获得前四个bit--是否全1为ADTS的开头
		//为什么要转为unsigned int????----避免位扩展问题
		res->id = ((unsigned int)in[1] & 0x08) >> 3;	//第二个字节与0x80进行与运算，获得第13位bit（就整个连续的in流来说）对应的值
		res->layer = ((unsigned int)in[1] & 0x06) >> 1;	//第二个字节与0x60进行与运算之后，右移1位，获得第14、15个比特的值
		res->protectionAbsent = (unsigned int)in[1] & 0x01;
		res->profile = ((unsigned int)in[2] & 0xc0) >> 6;
		res->samplingFreqIndex = ((unsigned int)in[2] & 0x3c) >> 2;
		res->privateBit = ((unsigned int)in[2] & 0x02) >> 1;
		res->channelCfg = ((((unsigned int)in[2] & 0x01) << 2) | (((unsigned int)in[3] & 0xc0) >> 6));
		res->originalCopy = ((unsigned int)in[3] & 0x20) >> 5;
		res->home = ((unsigned int)in[3] & 0x10) >> 4;
		res->copyrightIdentificationBit = ((unsigned int)in[3] & 0x08) >> 3;
		res->copyrightIdentificationStart = (unsigned int)in[3] & 0x04 >> 2;
		res->accFrameLength = (((((unsigned int)in[3]) & 0x03) << 11) |
			(((unsigned int)in[4] & 0xFF) << 3) |
			((unsigned int)in[5] & 0xE0) >> 5);
		res->adtsBufferFullness = (((unsigned int)in[5] & 0x1f) << 6 |
			((unsigned int)in[6] & 0xfc) >> 2);
		res->numberOfRawDataBlockInFrame = ((unsigned int)in[6] & 0x03);
		return 0;
	}
	else {
		std::cerr << "failed to parse adts header..." << std::endl;
		return -1;
	}
}

int rtpSendAACFrame_UDP(int sock, const char* ip, const uint16_t& port, struct RtpPacket* rtpPacket, uint8_t* frame, const uint32_t& framesize) {
	//打包文档：https://blog.csdn.net/yangguoyu8023/article/details/106517251/
	//这里的frame完全是adts的包体，不包含头部的
	int ret;

	rtpPacket->payload[0] = 0x00;
	rtpPacket->payload[1] = 0x10;
	rtpPacket->payload[2] = (framesize & 0x1FE0) >> 5;	//高8位
	rtpPacket->payload[3] = (framesize & 0x1F) << 3;	//低5位

	memcpy(rtpPacket->payload + 4, frame, framesize);

	ret = rtpSendPacketOverUdp(sock, ip, port, rtpPacket, framesize + 4);
	if (ret < 0) {
		std::cerr << "failed to send rtp packet..." << std::endl;
		return -1;
	}
	rtpPacket->rtpHeader.seq++;
	//音画不同步问题：帧数不对应
	/*
	 * 如果采样频率是44100
	 * 一般AAC每个1024个采样为一帧
	 * 所以一秒就有 44100 / 1024 = 43帧
	 * 时间增量就是 44100 / 43 = 1025
	 * 一帧的时间为 1 / 43 = 23ms
	 */
	rtpPacket->rtpHeader.timestamp += AAC_TIMESTAMP_;
	return 0;
}

int rtpSendAACFrame_TCP(int clientSockFd, struct RtpPacket* rtpPacket, uint8_t* frame, const uint32_t& frameSize) {
	int ret = 0;

	rtpPacket->payload[0] = 0x00;
	rtpPacket->payload[1] = 0x10;
	rtpPacket->payload[2] = (frameSize & 0x1FE0) >> 5;	//高8位
	rtpPacket->payload[3] = (frameSize & 0x1F) << 3;	//低5位

	memcpy(rtpPacket->payload + 4, frame, frameSize);

	ret = rtpSendPacketOverTcp(clientSockFd, rtpPacket, frameSize + 4, 0x02);	//0x20 总共有两路--视频流与音频流，视频流RTP和RTCP占两个通道（0&1），音频流占两个通道（2&3），因此从2开头
	if (ret < 0) {
		std::cout <<__FUNCTION__<< " :failed to send rtp packet" << std::endl;
		return -1;
	}
	rtpPacket->rtpHeader.seq++;
	/*
	 * 如果采样频率是44100
	 * 一般AAC每个1024个采样为一帧
	 * 所以一秒就有 44100 / 1024 = 43帧
	 * 时间增量就是 44100 / 43 = 1025
	 * 一帧的时间为 1 / 43 = 23ms
	 */
	rtpPacket->rtpHeader.timestamp += AAC_TIMESTAMP_;

	return 0;
}