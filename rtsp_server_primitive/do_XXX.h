#ifndef DOXXX__H
#define DOXXX__H
#include <iostream>
#define SERVER_RTP_PORT 55532
#define SERVER_RTCP_PORT 55533
int handlCmd_OPTIONS(std::string& result, int cseq);
int handlCmd_DESCIRBE_UDP(std::string& result, int cseq, std::string& url);
int handlCmd_DESCIRBE_TCP(std::string& result, int cseq, std::string& url);
int handlCmd_SETUP_UDP(std::string& result, int cseq, int clientRtpPort);
int handlCmd_SETUP_TCP(std::string& result, int cseq);
int handlCmd_PLAY(std::string& result, int cseq);
#endif