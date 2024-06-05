#ifndef RTP__H
#define RTP__H
#include <cstdint>
#include <iostream>
#include <memory>
#pragma comment(lib,"ws2_32.lib")
#define WIN32_LEAN_AND_MEAN

#define RTP_HEADER_SIZE 12 //12��byte
#define SERVER_RTP_PORT 55532
#define SERVER_RTCP_PORT 55533
#define RTP_VERSION 2
#define RTP_PAYLOAD_TYPE_H264 96
#define RTP_MAX_PKT_SIZE 1400
#define RTP_PAYLOAD_TYPE_AAC 97
#define H264_MAC_ 30
#define SAMPLE_RATE 44100	//����Ƶ��
#define SAMPLE_POINT 1024	//һ֡�������ٸ�������
#define AAC_MAC_ SAMPLE_RATE/SAMPLE_POINT	//һ���֡��
#define AAC_TIMESTAMP_ SAMPLE_RATE/AAC_MAC_
#define H264_TIMESTAMP_ 90000 / H264_MAC_	//9000/H264_MAC_��9000��h264ʱ�������
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
	/*�ο�RTP���ķ���*/
	uint8_t csrcLen : 4;	//CSRC��������ռ4λ��ָʾCSRC��ʶ���ĸ���
	uint8_t extension : 1;	//ռ1λ�����X=1������RTP����ͷ�����һ����չ��ͷ
	uint8_t padding : 1;	//����־��ռ1λ�����P=1�����ڸñ��ĵ�β�����һ�����߶������İ�λ�飬���ǲ�����Ч�غɵĲ���
	uint8_t version : 2;	//RTPЭ��İ汾�ţ�ռ2λ����ǰЭ��汾��Ϊ2
	
	/*byte 1*/
	uint8_t payloadType : 7;	//��Ч�غ����ͣ�ռ7λ������˵��RTP��������Ч�غɵ����ͣ���GSM��Ƶ��JPEMͼ��ȵ�
	uint8_t marker : 1;	//��ǣ�ռ1λ����ͬ����Ч�غ��в�ͬ�ĺ��壬������Ƶ�����һ֡�Ľ�����������Ƶ����ǻỰ�Ŀ�ʼ

	/*byte 2,3*/
	uint16_t seq;	//ռ16λ�����ڱ�ʶ�����������͵�RTP���ĵ����кţ�ÿ����һ�����ģ����к���1��������ͨ�����к�����ⱨ�ĵĶ�ʧ��������������ģ��ָ�����

	/*byte 4--7*/
	uint32_t timestamp;	//ռ32λ��ʱ�����ӳ�˸�RTP���ĵĵ�һ����λ��Ĳ���ʱ�̣�������ʹ��ʱ����������ӳٺ��ӳٶ�����������ͬ������

	/*byte 8--11*/
	uint32_t ssrc;	//ռ32λ�����ڱ�ʶͬ����Դ���ñ�ʶ�������ѡ��ģ��μ�ͬһ����Ƶ���������ͬ����Դ��������ͬ��ͬ����Դ��

	/*��׼��RTP Header �����ܴ��� 0-15����Լ��Դ(CSRC)��ʶ��

   ÿ��CSRC��ʶ��ռ32λ��������0��15����ÿ��CSRC��ʶ�˰����ڸ�RTP������Ч�غ��е�������Լ��Դ

   */
};
struct RtpPacket {
	struct RtpHeader rtpHeader;
	uint8_t payload[0];
};

struct AdtHeaders {
	unsigned int syncword;	//12bitͬ����"1111 1111 1111",һ��ADTS֡�Ŀ�ʼ
	unsigned int id;	//1bit 0����MPEG-4��1����MPEG-2
	unsigned int layer;	//2bit ����Ϊ0��
	unsigned int protectionAbsent;	//1bit 1����û��CRC��0������CRC
	unsigned int profile;	//1bit AAC����MPEG-2 AAC�ж�����3��profile��MPEG-4�ж�����6��profile��
	unsigned int samplingFreqIndex;	//4bit ������
	unsigned int privateBit;	//1bit ����ʱ������Ϊ0������ʱ�����
	unsigned int channelCfg;	//3bit ��������
	unsigned int originalCopy;	//1bit ����ʱ������Ϊ0������ʱ�����
	unsigned int home;	//1bit ����ʱ������Ϊ0������ʱ�����

	//�����Ϊ�ı�Ĳ�����ÿһ֡����ͬ
	unsigned int copyrightIdentificationBit;	//1bit ����ʱ������Ϊ0������ʱ�����
	unsigned int copyrightIdentificationStart;	//1bit ����ʱ������Ϊ0������ʱ�����
	unsigned int accFrameLength;	//13bit һ��ADTS֡�ĳ��Ȱ���ADTS��ͷ��AACԭʼ��
	unsigned int adtsBufferFullness;	//11bit �����������ȣ�0x7F˵���ǿɱ������������Ҫ���ֶΡ�CBR������Ҫ���ֶΡ���ͬ������ʹ�������ͬ�������ʹ����Ƶ�����ʱ����Ҫע��

	/* number_of_raw_data_blocks_in_frame
	 * ��ʾADTS֡����number_of_raw_data_blocks_in_frame + 1��AACԭʼ֡
	 * ����˵number_of_raw_data_blocks_in_frame == 0
	 * ��ʾ˵ADTS֡����һ��AAC���ݿ鲢����˵û�С�(һ��AACԭʼ֡����һ��ʱ����1024���������������)
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
