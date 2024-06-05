#include "do_XXX.h"
#include <iostream>
#include <string>
#include <sstream>
int handlCmd_OPTIONS(std::string& result, int cseq) {
	std::ostringstream temp;
	//std::cout << __FUNCTION__ << std::endl;
	temp << "RTSP/1.0 200 OK\r\n"
		<< "CSeq: " << cseq << "\r\n"
		<< "Public: OPTIONS, DESCRIBE, SETUP, PLAY\r\n"
		<< "\r\n";
	result = temp.str();//会返回当前流的内容但不会清除流的内容
	temp.str("");//手动清除流的内容
	return 0;
}
int handlCmd_DESCIRBE_UDP(std::string& result, int cseq, std::string& url) {
	//从一个形如"rtsp:// <你的IP地址> :  <端口号>  "获取IP地址
	//std::cout << __FUNCTION__ << std::endl;
	std::string localIP;
	std::size_t pos1 = url.find("rtsp://");
	if (std::string::npos == pos1) {
		std::cerr << __FUNCTION__ << " cant't find IP..." << std::endl;
		return 1;
	}
	pos1 += 7;//跳过"rtsp://"
	std::size_t pos2 = url.find(":",pos1);
	if (std::string::npos == pos2) {
		std::cerr << __FUNCTION__ << " can't find port..." << std::endl;
		return 1;
	}
	localIP = url.substr(pos1, (pos2 - pos1));

	//构造1
	std::ostringstream sdp;

	std::string _sdp;
	sdp << "v=0\r\n"
		<< "o=- 9" << time(NULL) << " 1 IN IP4 " << localIP << "\r\n"
		<< "t=0 0\r\n"
		<< "a=control:*\r\n"
		<< "m=video 0 RTP/AVP 96\r\n"
		<< "a=rtpmap:96 H264/90000\r\n"
		<< "a=control:track0\r\n";
	_sdp = sdp.str();
	sdp.str("");
	//构造2
	sdp << "RTSP/1.0 200 OK\r\n"
		<< "Cseq: " << cseq << "\r\n"
		<< "Content-Base: " << url << "\r\n"
		<< "Content-type: application/sdp\r\n"
		<< "Content-length: " << _sdp.length() << "\r\n\r\n"
		<< _sdp;
	result = sdp.str();
	sdp.str("");
	return 0;
}

int handlCmd_DESCIRBE_TCP(std::string& result, int cseq, std::string& url) {
	//从一个形如"rtsp:// <你的IP地址> :  <端口号>  "获取IP地址
//std::cout << __FUNCTION__ << std::endl;
	std::string localIP;
	std::size_t pos1 = url.find("rtsp://");
	if (std::string::npos == pos1) {
		std::cerr << __FUNCTION__ << " cant't find IP..." << std::endl;
		return 1;
	}
	pos1 += 7;//跳过"rtsp://"
	std::size_t pos2 = url.find(":", pos1);
	if (std::string::npos == pos2) {
		std::cerr << __FUNCTION__ << " can't find port..." << std::endl;
		return 1;
	}
	localIP = url.substr(pos1, (pos2 - pos1));

	//构造1
	std::ostringstream sdp;

	std::string _sdp;
	sdp << "v=0\r\n"
		<< "o=- 9" << time(NULL) << " 1 IN IP4 " << localIP << "\r\n"
		<< "t=0 0\r\n"
		<< "a=control:*\r\n"
		<< "m=video 0 RTP/AVP/TCP 96\r\n"
		<< "a=rtpmap:96 H264/90000\r\n"
		<< "a=control:track0\r\n"
		<< "m=audio 1 RTP/AVP/TCP 97\r\n"
		<< "a=rtpmap:97 mpeg4-generic/44100/2\r\n"
		<< "a=fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=1210;\r\n"
		<< "a=control:track1\r\n";
	_sdp = sdp.str();
	sdp.str("");
	//构造2
	sdp << "RTSP/1.0 200 OK\r\n"
		<< "Cseq: " << cseq << "\r\n"
		<< "Content-Base: " << url << "\r\n"
		<< "Content-type: application/sdp\r\n"
		<< "Content-length: " << _sdp.length() << "\r\n\r\n"
		<< _sdp;
	result = sdp.str();
	sdp.str("");
	return 0;
}

int handlCmd_SETUP_UDP(std::string& result, int cseq, int clientRtpPort) {
	//std::cout << __FUNCTION__ << std::endl;
	std::ostringstream temp;
	temp << "RTSP/1.0 200 OK\r\n"
		<< "CSeq: " << cseq << "\r\n"
		<< "Transport: RTP/AVP;unicast;client_port=" << clientRtpPort << "-" << (clientRtpPort + 1)<<";"
		<< "server_port=" << SERVER_RTP_PORT << "-" << SERVER_RTCP_PORT << "\r\n"
		<< "Session: 66334873\r\n"
		<< "\r\n";
	result = temp.str();
	temp.str("");
	return 0;
}

int handlCmd_SETUP_TCP(std::string& result, int cseq) {
	std::ostringstream oos;
	if (3 == cseq) {
		oos << "RTSP/1.0 200 OK\r\n"
			<< "CSeq: " << cseq << "\r\n"
			<< "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n"
			<< "Session: 66334873\r\n"
			<< "\r\n";
		result = oos.str();
		oos.str("");
	}
	else if(4==cseq){
		oos << "RTSP/1.0 200 OK\r\n"
			<< "CSeq: " << cseq << "\r\n"
			<< "Transport: RTP/AVP/TCP;unicast;interleaved=2-3\r\n"
			<< "Session: 66334873\r\n"
			<< "\r\n";
		result = oos.str();
		oos.str("");
	}
	oos.str("");
	return 0;
}

int handlCmd_PLAY(std::string& result, int cseq) {
	//std::cout << __FUNCTION__ << std::endl;
	std::ostringstream temp;
	temp << "RTSP/1.0 200 OK\r\n"
		<< "CSeq: " << cseq << "\r\n"
		<< "Range: npt=0.000-\r\n"
		<< "Session: 66334873; timeout=10\r\n"
		<< "\r\n";
	result = temp.str();
	temp.str("");
	return 0;
}