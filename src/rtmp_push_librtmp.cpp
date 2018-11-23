#include "ProControl.h"
#include "rtmp_push_librtmp.h"
#include "LibRtmpTool.h"
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <queue>

extern RtmpInfo rtmp_info;
extern std::queue <MediaDataStruct> *video_buf_queue;
extern pthread_mutex_t video_buf_queue_lock;
extern std::queue <MediaDataStruct> *audio_buf_queue_faac;
extern pthread_mutex_t audio_buf_queue_lock_faac;
extern void LOG(bool flag, std::string str);

extern bool push_flag;

int push_vid_fps = 0;
int push_aud_fps = 0;

extern LibRtmpClass *libRtmp;
extern bool get_audio;

long vid_pts = 0;
long aud_pts = 0;

bool pushing_vid = false;
bool pushing_aud = false;
int vid_duration = 1000 / DEFAULT_FPS;

void *LibRtmpPushVideoFun(void *ptr){
    std::string function = __FUNCTION__;
    int ret = -1;
    int wait_num = 0;
    struct timeval start_tv = {0, 0};
    struct timeval current_tv;
    LOG(true, "In " + function);
    while(1){
        if(!push_flag){
            usleep(1000);
            if(wait_num % 1000 == 0 && wait_num > 0){
                LOG(true, function + " wait push");
            }
            wait_num++;
            vid_pts = 0;
            aud_pts = 0;
            start_tv = {0, 0};
            while(video_buf_queue->size() > 0){
                pthread_mutex_lock(&video_buf_queue_lock);
                MediaDataStruct media_data = video_buf_queue->front();
                video_buf_queue->pop();
                pthread_mutex_unlock(&video_buf_queue_lock);
                free(media_data.buff);
                media_data.buff = NULL;
            }
            continue;
        }
        if(video_buf_queue->size() <= 0){
            usleep(1000);
            if(wait_num % 1000 == 0 && wait_num > 0){
            LOG(true, function + " no video data");
            wait_num = 0;
            }
            wait_num++;
            continue;
        }
        wait_num = 0;
        pthread_mutex_lock(&video_buf_queue_lock);
        MediaDataStruct media_data = video_buf_queue->front();
        video_buf_queue->pop();
        pthread_mutex_unlock(&video_buf_queue_lock);
#if AUDIO_STATUS
	if(get_audio == false){
		if(media_data.buff){
			free(media_data.buff);
			media_data.buff = NULL;
		}
		continue;
	}
#endif
        NaluUnit naluUnitData;
        naluUnitData.data = NULL;
        ret = libRtmp->ReadOneNaluFromBuf(&naluUnitData, media_data.buff, media_data.len);
        if(ret < 0){
		char ret_c[10];
		sprintf(ret_c, "%d", ret);
            LOG(false, function + " ReadFirstNaluFromBuf failed " + ret_c);
            continue;
        }
        int bKeyframe  = (naluUnitData.type == 0x05) ? true : false;
        if(start_tv.tv_sec == 0){
            vid_pts += 1000 / DEFAULT_FPS;
            start_tv = media_data.tv;
            //gettimeofday(&start_tv, NULL);
        }else{
            //gettimeofday(&current_tv, NULL);
            long t1 = media_data.tv.tv_sec * 1000 * 1000 + media_data.tv.tv_usec;
            //long t1 = current_tv.tv_sec * 1000 * 1000 + current_tv.tv_usec;
            long t2 = start_tv.tv_sec * 1000 * 1000 + start_tv.tv_usec;
            vid_pts = (t1 - t2) / 1000;
        }
        //vid_pts += vid_duration*2;
        while(pushing_aud){
            usleep(100);
        }
        pushing_vid = true;
        ret = libRtmp->SendH264Packet(&naluUnitData,bKeyframe,vid_pts);
        if(ret == 1){
            printf("In %s send video data success pts %ld len %d\n", __FUNCTION__, vid_pts, media_data.len);
        }else{
            printf("In %s send video data failed pts %ld error num %d\n", __FUNCTION__, vid_pts, ret);
            LOG(false, function + " send video data failed");
        }
        pushing_vid = false;
        push_vid_fps++;
        if(naluUnitData.data){
            free(naluUnitData.data);
            naluUnitData.data = NULL;
        }
        if(media_data.buff){
            free(media_data.buff);
            media_data.buff = NULL;
        }
    }
}

void *LibRtmpPushAudioFun(void *ptr){
    std::string function = __FUNCTION__;
    int wait_num = 0;
    int ret = -1;
    struct timeval start_tv = {0, 0};
    int audio_duration = 1000 * 1024 / AUDIO_RATE;
    LOG(true, "In " + function);
    while(1){
        if(!push_flag){
            usleep(1000);
            if(wait_num % 3000 == 0 && wait_num > 0){
                LOG(true, function + " wait push");
            }
            wait_num++;
            vid_pts = 0;
            aud_pts = 0;
            while(audio_buf_queue_faac->size() > 0){
                pthread_mutex_lock(&audio_buf_queue_lock_faac);
                MediaDataStruct media_data = audio_buf_queue_faac->front();
                audio_buf_queue_faac->pop();
                pthread_mutex_unlock(&audio_buf_queue_lock_faac);
                if(media_data.buff){
                    free(media_data.buff);
                    media_data.buff = NULL;
                }
            }
            continue;
        }
        if(audio_buf_queue_faac->size() <= 0){
            usleep(1000);
            if(wait_num % 1000 == 0 && wait_num > 0){
                LOG(true, function + " no audio data");
                wait_num = 0;
            }
            wait_num++;
            continue;
        }
	wait_num = 0;
        pthread_mutex_lock(&audio_buf_queue_lock_faac);
        MediaDataStruct media_data = audio_buf_queue_faac->front();
        audio_buf_queue_faac->pop();
        pthread_mutex_unlock(&audio_buf_queue_lock_faac);
        /*if(start_tv.tv_sec == 0){
            aud_pts += audio_duration;
            start_tv = media_data.tv;
        }else{
            long t1 = media_data.tv.tv_sec * 1000 * 1000 + media_data.tv.tv_usec;
            long t2 = start_tv.tv_sec * 1000 * 1000 + start_tv.tv_usec;
            aud_pts = (t1 - t2) / 1000;
        }*/
        aud_pts += audio_duration;
        while(pushing_vid){
            usleep(100);
        }
        pushing_aud = true;
        ret = libRtmp->SendAACData(media_data.buff, media_data.len, aud_pts);
        if(ret == 1){
            printf("In %s send audio data success pts %ld len %d\n", __FUNCTION__, aud_pts, media_data.len);
        }else{
            LOG(false, function + " send audio data failed");
            printf("In %s send audio data failed pts %ld\n", __FUNCTION__, aud_pts);
        }
        pushing_aud = false;
        push_aud_fps++;
        if(media_data.buff){
            free(media_data.buff);
            media_data.buff = NULL;
        }
    }
}


void *LibRtmpVideoFpsFun(void *ptr){
    std::string function = __FUNCTION__;
    struct timeval tv1 = {0, 0};
    struct timeval tv2 = {0, 0};
    int tem = 0;
    while(1){
        if(!push_flag){
            push_vid_fps = 0;
            push_aud_fps = 0;
            usleep(100);
            continue;
        }
        if(tv1.tv_sec == 0){
            gettimeofday(&tv1, NULL);
        }else{
            gettimeofday(&tv2, NULL);
            long t = tv2.tv_sec * 1000 * 1000 + tv2.tv_usec - (tv1.tv_sec * 1000 * 1000 + tv1.tv_usec);
            if(t >= 1000 * 1000){
                char push_vid_fps_c[10];
                char push_aud_fps_c[10];
                char default_fps_c[10];
                char vid_pts_c[10];
                char aud_pts_c[10];
                sprintf(push_vid_fps_c, "%d", push_vid_fps);
                sprintf(default_fps_c, "%d", DEFAULT_FPS);
                sprintf(push_aud_fps_c, "%d", push_aud_fps);
                sprintf(vid_pts_c, "%d", vid_pts);
                sprintf(aud_pts_c, "%d", aud_pts);
		LOG(true, function + "live video fps: " + push_vid_fps_c + " fps Default fps:" + default_fps_c + " fps push aud fps :" +push_aud_fps_c + "video pts:"+ vid_pts_c +" audio pts:" + aud_pts_c + "+++++++++++");
                //printf(LIGHT_BLUE "###########video output frame time: %d fps \r" NONE,push_vid_fps);
                fflush(stdout);
                tv1 = {0, 0};
                push_vid_fps = 0;
                push_aud_fps = 0;
            }else{
                usleep(10);
            }
        }

    }
}
