#include "ProTool.h"
#include "bcrypt.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <curl/curl.h>
#include <json/json.h>


extern void LOG(bool flag, std::string str);

bool GetMacAddr(const char* dev_name,std::string& mac_addr)
{   
	std::string function = __FUNCTION__;
	if(!dev_name){ 
		LOG(false, function + " dev name is NULL");
		return false;
	}
	struct ifreq ifreq;
	int sock;
	if((sock=socket(AF_INET,SOCK_STREAM,0))<0)
	{   
		perror("socket");
		LOG(false, function + " socket ");
		return false;
	}
	strcpy(ifreq.ifr_name,dev_name);
	if(ioctl(sock,SIOCGIFHWADDR,&ifreq)<0)
	{   
		perror("ioctl");
		LOG(false, function + " ioctl");
		return false;
	}
	char temp_mac[64];
	sprintf(temp_mac,"%02x%02x%02x%02x%02x%02x",
			(unsigned char)ifreq.ifr_hwaddr.sa_data[0],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[1],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[2],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[3],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[4],
			(unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
	mac_addr = temp_mac;
	return true;
}

bool CURL_Get_Json_And_parse(const char *key, RtmpInfo *rtmp_info){
	std::string function = __FUNCTION__;
	CURL *curl = curl_easy_init();
	if(curl){
		std::string url = "";
		std::string rtmp_json;
		url = url + "http://dev.vvtrobot.com/video/get_push_url/?serial_no=" + key;
		printf("serve url:%s\n", url.c_str());
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rtmp_json);
		CURLcode res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		if(res != CURLE_OK){
			printf("get json data failed \n");
			return false;
		}
		printf("get json data success, json data:%s\n", rtmp_json.c_str());
		Json::Reader read;
		Json::Value data;
		if(!read.parse(rtmp_json.c_str(), data))
		{
			printf("----(%s)--(%s)----(%d)-json parse failed!-\n",__FILE__,__FUNCTION__,__LINE__);
			LOG(false, function + " json parse failed");
			return false;
		}
		rtmp_info->rtmp_info[0].rtmp_url_live = data["push_url"].asString().c_str();
		
		return true;
	}
}

bool CURL_Get_Json_And_parse2(const char *key, RtmpInfo * rtmp_info){
	std::string function = __FUNCTION__;
	// http get rtmp json info 
	CURL *curl = NULL;
	CURLcode res;
	struct curl_slist *headers = NULL;
	std::string rtmp_json;
	int ret = 0;
	std::string fingerprint("Fingerprint: ");       //headers member
	std::string token("Token: ");   //filled with hash hw
	char tokenbuf[96];
	char salt[BCRYPT_HASHSIZE];

	curl = curl_easy_init();
	if(curl)
	{
		std::string url;
		url = url+"http://dev.vvtrobot.com/video/get_push_url/" + key;
		bcrypt_gensalt(14, salt);
		bcrypt_hashpw(key, salt, tokenbuf);
		fingerprint += key;
		token += tokenbuf;
		headers = curl_slist_append(headers, fingerprint.c_str());
		headers = curl_slist_append(headers, token.c_str());
		res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		if(res != CURLE_OK)
		{
			printf("Damn it , curl add header failed:%s!\n", curl_easy_strerror(res));
			LOG(false, function + " curl add header failed");
			return false;
		}
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rtmp_json);
		// data now holds response
		// write curl response to string variable..
		res = curl_easy_perform(curl);
		/* Check for errors */
		if(res != CURLE_OK){
			fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
			LOG(false, function + "curl easy perform failed");
			return false;
		}
		/* always cleanup */
		curl_easy_cleanup(curl);
	}else{
		printf("----(%s)--(%s)----(%d)-curl_easy_init failed!-\n",__FILE__,__FUNCTION__,__LINE__);
		LOG(false, function + " curl easy init failed");
		return false;
	}
	printf("fingerprint:%s \n", fingerprint.c_str());
	printf("token:%s\n", token.c_str());
	printf("jsonData:\n%s \n", rtmp_json.c_str());
	//parse json
/*
	Json::Reader read;
	Json::Value  root;
	if (!read.parse(rtmp_json.c_str(), root))
	{
		printf("----(%s)--(%s)----(%d)-json parse failed!-\n",__FILE__,__FUNCTION__,__LINE__);
		LOG(false, function + " json parse failed");
		return false;
	}
	int code = root["return_code"].asInt();
	if(code != 0){
		printf("return_code = %d , is not 0\n",code);
		LOG(false, function + " return code is not 0 ");
		return false;
	}
	Json::Value detail = root["detail"];
	printf("live_push_rul:%s\n", detail["live_push_url"].asString().c_str());
	std::string urlstr = detail["live_push_url"].asString();
	int po = urlstr.find("publish");
	int pos = urlstr.find("?");
	rtmp_info->rtmp_info[0].rtmp_url_live = urlstr.substr(0, pos);
	printf("rtmp_url_live: %s \n", rtmp_info->rtmp_info[0].rtmp_url_live.c_str());
	char dev_name_[16];
	sprintf(dev_name_,"/dev/video0");
	rtmp_info->rtmp_info[0].dev_name = dev_name_;*/
	return true;
}

size_t writeToString(void *ptr, size_t size, size_t count, void *stream)
{
    ((std::string*)stream)->append((char*)ptr, 0, size* count);
     return size* count;
}

int GetRTMPUrlInfo(RtmpInfo& rtmp_info){
	std::string function = __FUNCTION__;
	memset(&rtmp_info,0,sizeof(RtmpInfo));
	std::string dev_id;
	if(!GetMacAddr("wlan0", dev_id))
	{
		printf("----(%s)--(%s)----(%d)-mac GetMacAddr failed!-\n",__FILE__,__FUNCTION__,__LINE__);
		LOG(false, " GetMacAddr failed");
		return -1;
	}
	if(!CURL_Get_Json_And_parse(dev_id.c_str(),&rtmp_info))
	{
		printf("----(%s)--(%s)----(%d)-CURL_Get_Json_And_parse failed!(%s)-\n",__FILE__,__FUNCTION__,__LINE__,dev_id.c_str());
		LOG(false, " Get rtmp url failed");
		return -1;
	}
	return 0;
}

