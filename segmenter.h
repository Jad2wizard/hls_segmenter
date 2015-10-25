#ifndef _SEGMENTER_H
#define _SEGMENTER_H
#include<stdio.h>
#include<stdlib.h>
#include "M3u8Util.h"
#include "UserCertification.h"
#define PRINT_LOG 1
#if PRINT_LOG
	FILE* fp_log;
#endif
typedef unsigned char uint8_t;
enum stream_type{AUDIO_STREAM, VIDEO_STREAM};
typedef struct option_t{
	int input_file;
	char* input_filename;
	char* output_filename;
	float segment_duration;
	short hls_list_size;
	char prefix[MAX_USER_NAME_LENGTH+1];
	uint8_t* extra_data;
	char* live_url;
	char* ondemand_url;
} option;
typedef struct stream_t{
	int video_pid;
	int audio_pid;
	int pmt_pid;
	char prefix[MAX_USER_NAME_LENGTH+1];
	char* live_url;
	char* ondemand_url;
	double segment_time;
	double prev_segment_time;
	int getPID;
	int ts_file_index;
	short hls_list_size;
	float segment_duration;
	int frame_rate_v;
	int frame_rate_a;
	uint8_t* extra_data;
	FILE* live_file_pointer;
	FILE* ondemand_file_pointer;
}stream;
int parseOneTS(uint8_t* buf, stream* st, LiveM3u8* livem3u8); 
void* segmenter(void*);
void initOption(option* opt, char** argv, int argc);
void setDefaultOption(option*, char*, int flag);
void printUsage();
#endif
