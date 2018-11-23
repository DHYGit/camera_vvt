#ifndef __LIBRTMPTOOL_H__
#define __LIBRTMPTOOL_H__
#include <librtmp/rtmp.h>  
#include <librtmp/log.h>
#include <librtmp/amf.h>
#include <pthread.h>
#include <iostream>

#define RTMP_HEAD_SIZE   sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE

struct RTMPMetadata{  
    // video, must be h264 type   
    unsigned int    nWidth;  
    unsigned int    nHeight;  
    unsigned int    nFrameRate;      
    unsigned int    nSpsLen;  
    unsigned char   Sps[50];  
    unsigned int    nPpsLen;  
    unsigned char   Pps[50];
    unsigned int    firstIFrameLen;  
    unsigned char   *firstIFrame;
    bool            first_i_flag;

}; 

struct NaluUnit  
{  
    int type;  
    int size;  
    unsigned char *data;
    unsigned char spsdata[100];
    unsigned char ppsdata[100];
    int spslen;
    int ppslen;
    int flag;  
};

class LibRtmpClass{
public:
    RTMP *rtmp;
    RTMPMetadata    metaData;
    pthread_mutex_t mutex;
    NaluUnit *naluUnit;
    std::string m_url;
    int InitSockets(); 
    void CleanupSockets(); 
    void LibRTMP_Close();  
    int LibRTMP_Connect(const char* url);
    RTMPMetadata GetVideoSpsPps(unsigned char *buff, int len);
    int ReadOneNaluFromBuf(NaluUnit *nalu, unsigned char *buff, int len);
    int SendVideoSpsPps(unsigned char *sps,int sps_len,unsigned char * pps,int pps_len);
    int SendH264Packet(NaluUnit *nalu,int bIsKeyFrame,unsigned int nTimeStamp);
    int SendAACHeader(unsigned char *spec_buf, int spec_len);
    int SendAACData(unsigned char * data,int len, unsigned int nTimestamp);
    int SendPacket(unsigned int nPacketType,unsigned char *data,unsigned int size,unsigned int nTimestamp);
};
#endif
