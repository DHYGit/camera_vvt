#pragma once
#include "Error.h"
#include <iostream>
#include <stdio.h>
#include <sys/time.h>


#define DEBUG  0
#define USE_SOCK 0
#define SOFTENCODE 0
#define USE_FFMPEG  0
#define VIDEO_STATUS  1
#define AUDIO_STATUS  1
#define AUDIO_RATE 16000
#define DST_WIDTH 1280
#define DST_HEIGHT 720
#define ENCODER_PIXFMT  V4L2_PIX_FMT_H264
//#define ENCODER_PIXFMT  V4L2_PIX_FMT_H265
#define DEFAULT_FPS 25

#define VIDEOINDEX 0
#define AUDIOINDEX 1
struct MediaDataStruct{
    int len;
    int index;
    int duration;
    unsigned char *buff;
    struct timeval tv;
};

#define RTMP_STRUCT_NUM 4

typedef struct{
    std::string dev_id;     //camera id
    std::string dev_name;   //camera dev name
    std::string rtmp_url_live;  //remote rtmp url 
    std::string rtmp_url_record;
}RTMP_base;

typedef struct{
    RTMP_base  rtmp_info[RTMP_STRUCT_NUM];
    int   run_type;
}RtmpInfo;
