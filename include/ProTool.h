#ifndef __PROTOOL_H__
#define __PROTOOL_H__
#include "ProControl.h"
#include <iostream>

bool GetMacAddr(const char* dev_name,std::string& mac_addr);
bool CURL_Get_Json_And_parse(const char *key, RtmpInfo * rtmp_info);
size_t writeToString(void *ptr, size_t size, size_t count, void *stream);
int GetRTMPUrlInfo(RtmpInfo& rtmp_info);

#endif
