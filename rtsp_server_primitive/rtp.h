#ifndef RTP__H
#define RTP__H
#include <cstdint>
#include <iostream>
#include <memory>
#pragma comment(lib,"ws2_32.lib")
#define WIN32_LEAN_AND_MEAN

#define RTP_HEADER_SIZE 12 //12个byte
#define SERVER_RTP_PORT 55532
#define SERVER_RTCP_PORT 55533
#define RTP_VERSION 2
#define RTP_PAYLOAD_TYPE_H264 96
#define RTP_MAX_PKT_SIZE 1400
#define RTP_PAYLOAD_TYPE_AAC 97
#define H264_MAC_ 30
#define SAMPLE_RATE 44100	//采样频率
#define SAMPLE_POINT 1024	//一帧包含多少个采样点
#define AAC_MAC_ SAMPLE_RATE/SAMPLE_POINT	//一秒的帧数
#define AAC_TIMESTAMP_ SAMPLE_RATE/AAC_MAC_
#define H264_TIMESTAMP_ 90000 / H264_MAC_	//9000/H264_MAC_（9000是h264时间基数）
/*
  *    0                   1                   2                   3
  *    7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
  *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
  *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *   |                           timestamp                           |
  *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *   |           synchronization source (SSRC) identifier            |
  *   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
  *   |            contributing source (CSRC) identifiers             |
  *   :                             ....                              :
  *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *
  */
struct RtpHeader {
	/*byte 0*/
	/*参考RTP报文分析*/
	uint8_t csrcLen : 4;	//CSRC计数器，占4位，指示CSRC标识符的个数
	uint8_t extension : 1;	//占1位，如果X=1，则在RTP报文头部后跟一个扩展报头
	uint8_t padding : 1;	//填充标志，占1位，如果P=1，则在该报文的尾部填充一个或者多个额外的八位组，他们不是有效载荷的部分
	uint8_t version : 2;	//RTP协议的版本号，占2位，当前协议版本号为2
	
	/*byte 1*/
	uint8_t payloadType : 7;	//有效载荷类型，占7位，用于说明RTP报文中有效载荷的类型，如GSM音频，JPEM图像等等
	uint8_t marker : 1;	//标记，占1位，不同的有效载荷有不同的含义，对于视频，标记一帧的结束；对于音频，标记会话的开始

	/*byte 2,3*/
	uint16_t seq;	//占16位，用于标识发送者所发送的RTP报文的序列号，每发送一个报文，序列号增1。接受者通过序列号来检测报文的丢失情况，重新排序报文，恢复数据

	/*byte 4--7*/
	uint32_t timestamp;	//占32位，时间戳反映了该RTP报文的第一个八位组的采样时刻，接受者使用时间戳来计算延迟和延迟抖动，并进行同步机制

	/*byte 8--11*/
	uint32_t ssrc;	//占32位，用于标识同步信源。该标识符是随机选择的，参加同一个视频会议的两个同步信源不能有相同的同步信源。

	/*标准的RTP Header 还可能存在 0-15个特约信源(CSRC)标识符

   每个CSRC标识符占32位，可以有0～15个。每个CSRC标识了包含在该RTP报文有效载荷中的所有特约信源

   */
};
struct RtpPacket {
	struct RtpHeader rtpHeader;
	uint8_t payload[0];
};

struct AdtHeaders {
	unsigned int syncword;	//12bit同步字"1111 1111 1111",一个ADTS帧的开始
	unsigned int id;	//1bit 0代表MPEG-4，1代表MPEG-2
	unsigned int layer;	//2bit 必须为0、
	unsigned int protectionAbsent;	//1bit 1代表没有CRC，0代表有CRC
	unsigned int profile;	//1bit AAC级别（MPEG-2 AAC中定义了3种profile，MPEG-4中定义了6中profile）
	unsigned int samplingFreqIndex;	//4bit 采样率
	unsigned int privateBit;	//1bit 编码时候设置为0，解码时候忽略
	unsigned int channelCfg;	//3bit 声道数量
	unsigned int originalCopy;	//1bit 编码时候设置为0，解码时候忽略
	unsigned int home;	//1bit 编码时候设置为0，解码时候忽略

	//下面的为改变的参数即每一帧都不同
	unsigned int copyrightIdentificationBit;	//1bit 编码时候设置为0，解码时候忽略
	unsigned int copyrightIdentificationStart;	//1bit 编码时候设置为0，解码时候忽略
	unsigned int accFrameLength;	//13bit 一个ADTS帧的长度包括ADTS的头和AAC原始流
	unsigned int adtsBufferFullness;	//11bit 缓冲区充满度，0x7F说明是可变的码流，不需要此字段。CBR可能需要此字段。不同编码器使用情况不同，这个在使用音频编码的时候需要注意

	/* number_of_raw_data_blocks_in_frame
	 * 表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧
	 * 所以说number_of_raw_data_blocks_in_frame == 0
	 * 表示说ADTS帧中有一个AAC数据块并不是说没有。(一个AAC原始帧包含一段时间内1024个采样及相关数据)
	 */
	unsigned int numberOfRawDataBlockInFrame;	//2bit
};

void rtpHeaderInit(struct RtpPacket* rtpPacket, uint8_t csrLen,uint8_t extension,
	uint8_t padding,uint8_t version,uint8_t payloadTYpe,uint8_t marker,
	uint16_t seq,uint32_t timestamp,uint32_t ssrc);

int rtpSendPacketOverTcp(int clientSockFd,struct RtpPacket* rtpPacket,uint32_t datasize,char channel);

int rtpSendPacketOverUdp(int serverSockFd,const char* ip,uint16_t port,struct RtpPacket* rtpPacket,uint32_t datasize);

int getFrameFromH264File(FILE* fp, char* frame, int size);

inline int startcode3(char* buf);

inline int startcode4(char* buf);

char* findNextStartCode(char* buf, int len);

int rtpSendH264Frame_UDP(int serverRtpSockFd, const char* ip, int16_t port, 
				struct RtpPacket* rtpPacket, char* frame, uint32_t frameSize);

int rtpSendH264Frame_TCP(int clientSockFd, struct RtpPacket* rtpPacket, char* frame, const uint32_t& frameSize);

int parseAdtHeader(uint8_t* in, struct AdtHeaders* res);

int rtpSendAACFrame_UDP(int sock, const char* ip, const uint16_t& port,
	struct RtpPacket* rtpPacket, uint8_t* frame, const uint32_t& framesize);

int rtpSendAACFrame_TCP(int clientSockFd, struct RtpPacket* rtpPacket, uint8_t* frame, const uint32_t& frameSize);

#endif
