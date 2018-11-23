#include "LibRtmpTool.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

extern void LOG(bool flag, std::string str);

int LibRtmpClass::InitSockets()    
{    
#ifdef WIN32     
    WORD version;    
    WSADATA wsaData;    
    version = MAKEWORD(1, 1);    
    return (WSAStartup(version, &wsaData) == 0);    
#else     
    return TRUE;    
#endif     
}
int LibRtmpClass::LibRTMP_Connect(const char* url){
    rtmp = RTMP_Alloc();
    RTMP_Init(rtmp);
    //set connection timeout,default 30s
    rtmp->Link.timeout=5;
    if (RTMP_SetupURL(rtmp,(char*)url) == FALSE)
    {   
        printf("#####----(%s)--(%s)----(%d)--RTMP_SetupURL %s failed!--\n",__FILE__,__FUNCTION__,__LINE__, url);
        RTMP_Free(rtmp);
        return -1;
    }
    RTMP_EnableWrite(rtmp);
    if (RTMP_Connect(rtmp, NULL) == false)
    {   
        printf("#####----(%s)--(%s)----(%d)--RTMP_Connect failed!--\n",__FILE__,__FUNCTION__,__LINE__);
        RTMP_Free(rtmp);
        return -2;
    }   
    if(RTMP_ConnectStream(rtmp, 0) == false){
        printf("#####----(%s)--(%s)----(%d)--RTMP_ConnectStream failed!--\n",__FILE__,__FUNCTION__,__LINE__);
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        return -3;
    }
    pthread_mutex_init(&mutex, NULL);
    return 0;
}
RTMPMetadata LibRtmpClass::GetVideoSpsPps(unsigned char *buff, int len){
    RTMPMetadata meta_data;
    if(buff == NULL || len == 0){
        printf("data is NULL\n");
        return meta_data;
    }
    int sps_index = 0;
    int pps_index = 0;
    int index = 0;
    int nalu_size;
    while(index < len){
        if(buff[index++] == 0x00 &&buff[index++] == 0x00){ 
            if(buff[index++] == 0x01 && buff[index+1] == 0x67){
                goto gotsps_head;
            }else{
                index--;
                if(buff[index++] == 0x00 && buff[index++] == 0x01 && buff[index+1] == 0x67){
                    goto gotsps_head;
                }
            }
        }
        continue;
gotsps_head:
        sps_index = index;
        while(sps_index < len){
            if(buff[sps_index++] == 0x00 && buff[sps_index++] == 0x00){
                if(buff[sps_index++] == 0x01){
                    nalu_size = sps_index - 3 - index;
                    break;
                }else{
                    sps_index--;
                    if(buff[sps_index++] == 0x00 && buff[sps_index++] == 0x01){
                        nalu_size = sps_index - 4 - index;
                        break;
                    }
                }
            }
        }
        meta_data.nSpsLen = nalu_size;
        memcpy(meta_data.Sps, buff + index, nalu_size);
        index = sps_index;
        pps_index = index;
        int nalu_start = 0;
        while(pps_index < len){
            if(buff[pps_index++] == 0x00 && buff[pps_index++] == 0x00){
                if(buff[pps_index++] == 0x01){
                    nalu_start = 3;
                    goto gotnal;
                }else{
                    pps_index--;
                    if(buff[pps_index++] == 0x00 && buff[pps_index++] == 0x01){
                        nalu_start = 4;
                        goto gotnal;
                    }
                }
            }
            continue;
gotnal:
            meta_data.nPpsLen = pps_index - nalu_start - index;
            memcpy(meta_data.Pps, buff + index, meta_data.nPpsLen);
            index = pps_index;
            return meta_data;
        }
    }
}
int LibRtmpClass::ReadOneNaluFromBuf(NaluUnit *nalu, unsigned char *buff, int len){
    std::string function = __FUNCTION__;
    if(buff == NULL || len == 0){
        printf("data is NULL\n");
        return -1;
    }
    int index = 0;
    int ret;
    int nalustart;
    nalu->size = 0;
    while(index < len){
        if(buff[index++] == 0x00 && buff[index++] == 0x00){
            if(buff[index++] == 0x01){
                nalustart = 3;
                goto gotnal;
            }else{
                index--;
                if(buff[index++] == 0x00 && buff[index++] == 0x01){
                    nalustart = 4;
                    goto gotnal;
                }
            }
        }
        continue;
gotnal:
        nalu->type = buff[index]&0x1f;
        if(nalu->type == 0x07){//sps
            int sps_index = index;
            while(sps_index < len){
                if(buff[sps_index++] == 0x00 && buff[sps_index++] == 0x00){
                    if(buff[sps_index++] == 0x01){
                        nalu->spslen = sps_index - 3 - index;
                        break;
                    }else{
                        sps_index--;
                        if(buff[sps_index++] == 0x00 && buff[sps_index++] == 0x01){
                            nalu->spslen = sps_index - 4 - index;
                            break;
                        }
                    }
                }
            }
            memcpy(nalu->spsdata, buff + index, nalu->spslen);
            /*if(this->naluUnit->spslen != nalu->spslen){
                this->naluUnit->spslen = nalu->spslen;
                memset(this->naluUnit->spsdata, 0, 100);
                memcpy(this->naluUnit->spsdata, nalu->spsdata, nalu->spslen);
                this->naluUnit->flag = 1;
                LOG(true, function + " get sps data");
            }*/
            //index = sps_index;
            continue;
        }else if(nalu->type == 0x08){//pps
            int pps_index = index;
            while(pps_index < len){
                if(buff[pps_index++] == 0x00 && buff[pps_index++] == 0x00){
                    if(buff[pps_index++] == 0x01){
                        nalu->ppslen = pps_index - 3 - index;
                        break;
                    }else{
                        pps_index--;
                        if(buff[pps_index++] == 0x00 && buff[pps_index++] == 0x01){
                            nalu->ppslen = pps_index - 4 - index;
                            break;
                        }
                    }
                }
            }
            memcpy(nalu->ppsdata, buff + index, nalu->ppslen);
            /*if(this->naluUnit->ppslen != nalu->ppslen){
                this->naluUnit->ppslen = nalu->ppslen;
                memset(this->naluUnit->ppsdata, 0, 100);
                memcpy(this->naluUnit->ppsdata, nalu->ppsdata, nalu->ppslen);
                this->naluUnit->flag = 2;
                LOG(true, function + " get pps data");
            }*/
            //index = pps_index;    
            continue;
        }else if(nalu->type == 0x05 || nalu->type == 0x01){// I frame and P frame
            nalu->size = len - index;
            nalu->data = (unsigned char*)malloc(nalu->size);
            memcpy(nalu->data, buff + index, nalu->size);
            break;
        }
        continue;
    }
    return 0;
}
int LibRtmpClass::SendVideoSpsPps(unsigned char *sps,int sps_len,unsigned char * pps,int pps_len){
    RTMPPacket *spspacket = (RTMPPacket*)malloc(RTMP_HEAD_SIZE + 1024);
    unsigned char *body = NULL;
    int i = 0;
    memset(spspacket,0,RTMP_HEAD_SIZE+1024);
    spspacket->m_body = (char*)spspacket + RTMP_HEAD_SIZE;
    body = (unsigned char*)spspacket->m_body;
    body[i++] = 0x17;
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;
    /*AVCDecoderConfigurationRecord*/
    body[i++] = 0x01;
    body[i++] = sps[1];
    body[i++] = sps[2];
    body[i++] = sps[3];
    body[i++] = 0xff;
    /*sps*/
    body[i++]   = 0xe1;
    body[i++] = (sps_len >> 8) & 0xff;
    body[i++] = sps_len & 0xff;
    memcpy(&body[i],sps,sps_len);
    i +=  sps_len;
    /*pps*/
    body[i++]   = 0x01;
    body[i++] = (pps_len >> 8) & 0xff;
    body[i++] = (pps_len) & 0xff;
    memcpy(&body[i],pps,pps_len);
    i +=  pps_len;
    spspacket->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    spspacket->m_nBodySize = i;
    spspacket->m_nChannel = 0x04;
    spspacket->m_nTimeStamp = 0;
    spspacket->m_hasAbsTimestamp = 0;
    spspacket->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    spspacket->m_nInfoField2 = rtmp->m_stream_id;
    pthread_mutex_lock(&mutex); 
    int ret = RTMP_SendPacket(rtmp,spspacket,TRUE);
    pthread_mutex_unlock(&mutex);
    if(spspacket){
        free(spspacket);
        spspacket = NULL;
    }
    return ret;
}
int LibRtmpClass::SendH264Packet(NaluUnit *nalu,int bIsKeyFrame,unsigned int nTimeStamp){
	if(nalu->data == NULL&& nalu->size<11){  
		printf("%s %s %d data error\n",__FILE__,__FUNCTION__,__LINE__);
		return -1;  
	}

	unsigned char *body = (unsigned char*)malloc(nalu->size+9);
	memset(body,0,nalu->size+9);
	int i = 0;
	int ret = 0;
	if(bIsKeyFrame){
		body[i++] = 0x17;// 1:Iframe  7:AVC   
		body[i++] = 0x01;// AVC NALU   
		body[i++] = 0x00;  
		body[i++] = 0x00;  
		body[i++] = 0x00;
		//NALU size
		body[i++] = nalu->size>>24 &0xff;  
		body[i++] = nalu->size>>16 &0xff;  
		body[i++] = nalu->size>>8 &0xff;  
		body[i++] = nalu->size&0xff;
		//NALU data
		memcpy(&body[i],nalu->data,nalu->size);  
		ret = SendVideoSpsPps(nalu->spsdata,nalu->spslen,nalu->ppsdata,nalu->ppslen);
		if(ret == 1){
			//printf("In %s, send sps and pps success spslen:%d ppslen:%d \n", __FUNCTION__, nalu->spslen, nalu->ppslen);
			//printf("SPS Data:\n");
			//for(int index = 0; index < nalu->spslen; index++){
			//    printf("%02X  ", nalu->spsdata[index]);
			//}
			//printf("\n");
			//printf("PPS Data:\n");
			//for(int index = 0; index < nalu->ppslen; index++){
			//    printf("%02X  ", nalu->ppsdata[index]);
			//}
			//printf("\n");
		}
	}else{
		body[i++] = 0x27;// 2:Pframe  7:AVC   
		body[i++] = 0x01;// AVC NALU   
		body[i++] = 0x00;  
		body[i++] = 0x00;  
		body[i++] = 0x00;
		//NALU size
		body[i++] = nalu->size>>24 &0xff;  
		body[i++] = nalu->size>>16 &0xff;  
		body[i++] = nalu->size>>8 &0xff;  
		body[i++] = nalu->size&0xff;
		//NALU data
		memcpy(&body[i],nalu->data,nalu->size);
	}
	ret = SendPacket(RTMP_PACKET_TYPE_VIDEO,body,i + nalu->size,nTimeStamp);  
	free(body);  
	return ret; 
}

int LibRtmpClass::SendPacket(unsigned int nPacketType,unsigned char *data,unsigned int size,unsigned int nTimestamp){
    std::string function = __FUNCTION__;
    RTMPPacket* packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+size);
    memset(packet,0,RTMP_HEAD_SIZE);
    packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
    packet->m_nBodySize = size;
    memcpy(packet->m_body,data,size);
    packet->m_hasAbsTimestamp = 0;
    packet->m_packetType = nPacketType;
    packet->m_nInfoField2 = rtmp->m_stream_id;
    packet->m_nChannel = 0x04;//Video
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    if (RTMP_PACKET_TYPE_AUDIO ==nPacketType && size !=4)
    {
        packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    }
    packet->m_nTimeStamp = nTimestamp;
    int ret =0;
    if (RTMP_IsConnected(rtmp)){
        pthread_mutex_lock(&mutex);
        ret = RTMP_SendPacket(rtmp,packet,FALSE);
        if(ret == 0){//try to reconnect
            LibRTMP_Close();
            sleep(1);
            while(1)
            {
                if(0 != LibRTMP_Connect(m_url.c_str()))
                {
                    LOG(false, function + " reconnect to rtmp failed url:" + m_url);
                    usleep(1000 * 1000);
                    continue;
                }else{
                    LOG(true, function + " reconnect to rtmp success url:" + m_url);
                    break;
                }
            }
        }
        pthread_mutex_unlock(&mutex); 
    }
    free(packet);
    return ret;
}

int LibRtmpClass::SendAACHeader(unsigned char *spec_buf, int spec_len){
    std::string function = __FUNCTION__;
    if(!spec_buf || spec_len<=0){
        printf("In %s spec buf and len error\n", __FUNCTION__);
        return -1;
    }
    int len = spec_len;
    unsigned char * body;  
    RTMPPacket * packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+len+2);;  
    memset(packet,0,RTMP_HEAD_SIZE);
    packet->m_body = (char *)packet + RTMP_HEAD_SIZE;  
    body = (unsigned char *)packet->m_body;
    /*AF 00 + AAC RAW data*/  
    body[0] = 0xAF;  
    body[1] = 0x00;  
    memcpy(&body[2],spec_buf,len);
    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;  
    packet->m_nBodySize = len+2;  
    packet->m_nChannel = 0x05;  
    packet->m_nTimeStamp = 0;  
    packet->m_hasAbsTimestamp = 0;  
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;  
    packet->m_nInfoField2 = rtmp->m_stream_id;
    
    int ret =0;
    if (RTMP_IsConnected(rtmp)){
        pthread_mutex_lock(&mutex);
        ret = RTMP_SendPacket(rtmp,packet,TRUE);
        if(ret == 0){//try to reconnect
            LibRTMP_Close();
            usleep(1000 * 1000);
            while(1)
            {
                if(0 != LibRTMP_Connect(m_url.c_str()))
                {
                    LOG(false, function + " reconnect to rtmp failed url:" + m_url);
                    usleep(1000 * 1000);
                    continue;
                }else{
                    LOG(true, function + " reconnect to rtmp success url:" + m_url);
                    break;
                }
            }
        }
        pthread_mutex_unlock(&mutex);
    }
    free(packet);  
    return ret;
}

int LibRtmpClass::SendAACData(unsigned char * data,int len, unsigned int nTimestamp){
    std::string function = __FUNCTION__;
    if(!data || len<=7){
        printf("%s data error\n");
        return -1;
    }
    long timeoffset = 0;
    data += 7;
    len -= 7;
    if(len > 0){
        RTMPPacket * packet;  
        unsigned char * body;
        packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+len+2);  
        memset(packet,0,RTMP_HEAD_SIZE);  
        packet->m_body = (char *)packet + RTMP_HEAD_SIZE;  
        body = (unsigned char *)packet->m_body;
        /*AF 01 + AAC RAW data*/  
        body[0] = 0xAF;  
        body[1] = 0x01;  
        memcpy(&body[2],data,len); 

        packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;  
        packet->m_nBodySize = len+2;  
        packet->m_nChannel = 0x05;  
        packet->m_nTimeStamp = nTimestamp;  
        packet->m_hasAbsTimestamp = 0;  
        packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;  
        packet->m_nInfoField2 = rtmp->m_stream_id;
        int ret =0;
        if (RTMP_IsConnected(rtmp)){
            pthread_mutex_lock(&mutex);
            ret = RTMP_SendPacket(rtmp,packet,FALSE);
            if(ret == 0){//try to reconnect
                LibRTMP_Close();
                usleep(1000 * 1000);
                while(1)
                {
                    if(0 != LibRTMP_Connect(m_url.c_str()))
                    {
                        LOG(false, function + " reconnect to rtmp failed url:" + m_url);
                        usleep(1000 * 1000);
                        continue;
                    }else{
                        LOG(true, function + " reconnect to rtmp success url:" + m_url);
                        break;
                    }
                }
            }
            pthread_mutex_unlock(&mutex);
        }
        free(packet);
        return ret;
    }
}

void LibRtmpClass::LibRTMP_Close(){
    if(rtmp)  
    {  
        RTMP_Close(rtmp);  
        RTMP_Free(rtmp);  
        rtmp = NULL;  
    }  
    CleanupSockets();   
}

void LibRtmpClass::CleanupSockets()    
{    
#ifdef WIN32     
    WSACleanup();    
#endif     
}    



