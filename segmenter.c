#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include "segmenter.h"
#include "UserCertification.h"
#include <string.h>

#define TS_PACKET 188

int isKeyFrame(uint8_t* buf);
int openTSFile(int live_index, int ondemand_index, stream* st);


static int ff_index = 0;
static int pat_index = 0;
static int pmt_index = 0;
void* segmenter(void* op)
{
	option* opt = (option*)op;
	uint8_t* buffer = (uint8_t*)malloc(TS_PACKET);
	stream st;
	st.segment_duration = opt->segment_duration;
	st.hls_list_size = opt->hls_list_size;
	st.getPID=0;
	st.pmt_pid=-1;
	st.video_pid=-1;
	st.audio_pid=-1;
	st.frame_rate_v = 10;
	st.frame_rate_a = 20;
	st.ts_file_index = 0;
	st.live_url = opt->live_url;
	st.ondemand_url = opt->ondemand_url;
	st.extra_data = opt->extra_data;
	snprintf(st.prefix, MAX_USER_NAME_LENGTH, "%s",  opt->prefix);

	int chOpt = 1;
	int nErr=setsockopt(opt->input_file, IPPROTO_TCP, TCP_NODELAY, (char*)&chOpt, sizeof(int));
	if(nErr < 0)
	{
   		printf("setsockopt wrong: %s(error: %d)\n",strerror(errno), errno);
		deleteFromDb(&userDb, st.prefix, st.ts_file_index);
		close(opt->input_file);
		free(opt);
		return NULL;
	}



	//get user name
	char idPacket[TS_PACKET];
	int result = recv(opt->input_file, &idPacket, TS_PACKET, MSG_WAITALL);
	uint8_t nameLen = 0;
	if(result !=  TS_PACKET)
	{
		printf("Recv name length wrong\n");
   		printf("Recv name length wrong: %s(error: %d)\n",strerror(errno), errno);
		close(opt->input_file);
		free(opt);
		return NULL;
	}
	nameLen = idPacket[0];

	if(nameLen > MAX_USER_NAME_LENGTH)
	{
		printf("User name is too long %d\n", nameLen);
		close(opt->input_file);
		free(opt);
		return NULL;
	}
	snprintf(st.prefix, MAX_USER_NAME_LENGTH, "%s", idPacket + 1);
	st.prefix[nameLen] = '\0';

	printf("The user id is %s\n", idPacket + 1);



	userAccount* account = findAvailableSlot(&userDb, st.prefix, opt->input_file);
	if(account == NULL)
	{
		printf("The connection is full or the id already exits\n");
		close(opt->input_file);
		free(opt);
		return NULL;
	}
	//the ts file index continue after the index of the last connection;
	st.ts_file_index = account->segNum;

	openTSFile(0, 0, &st);
	LiveM3u8* livem3u8 = createLiveM3u8(st.hls_list_size);
	initLiveM3u8(livem3u8, st.segment_duration, st.prefix,st.live_url,st.ondemand_url);

	while(1)
	{
		result = recv(opt->input_file, buffer, TS_PACKET, MSG_WAITALL);
		if(result ==  TS_PACKET)
			parseOneTS(buffer, &st, livem3u8);
		else
		{
			printf("User %s exit\n", st.prefix);
			deleteFromDb(&userDb, st.prefix, st.ts_file_index);
			close(opt->input_file);
			destroy(livem3u8);
			free(opt);
			return NULL;
		}
		++livem3u8->tsSegNum;
		if(livem3u8->tsSegNum == TS_SEG_NUM_MAX)
		{
			//send the client num here
			char clientNum = getClientNum(&userDb);
			if(send(opt->input_file, &clientNum, 1, 0) < 0)
			{
				printf("Use %s can't send heartbeat info!\n", st.prefix);
			}
			//printf("Send HeartBeat\n");
			livem3u8->tsSegNum = 0;
		}
	}
	printf("%d\t%d\t%d\n",st.pmt_pid,st.video_pid,st.audio_pid);
	return NULL;
}

int parseOneTS(uint8_t* buf, stream* st, LiveM3u8* livem3u8)
{
	int pid =  ((buf[1]&0x1F)<<8|buf[2]);
	int is_key_frame = 0;
	int is_video = 0;
	int is_audio = 0;
	int is_frame_start = 0;
	static int frame_num = 0;
	if(st->getPID==0 && pid==0)
	{
		st->pmt_pid = 0x1FFF&(((buf[15]<<3)<<5)|buf[16]);
		st->getPID==1;
	}
	if(st->pmt_pid>0 && pid==st->pmt_pid)
	{
		int section_length = (buf[6]&0x0F)<<8 | buf[7];
		int pos = 17 + ((buf[15]&0x0F)<<8 | buf[16]);
		for(;pos<=(section_length-2);)
		{
			if(buf[pos]==0x1B)
			{
				st->video_pid = ((buf[pos+1]<<8)|buf[pos+2])&0x1FFF;
				pos=pos+5;
				continue;
			}
			if(buf[pos]==0x0F)
			{
				st->audio_pid = ((buf[pos+1]<<8)|buf[pos+2])&0x1FFF;
				pos=pos+5;
				continue;
			}
			pos++;
		}
	}
	if(pid == 0x11)
	{
		buf[3] = (buf[3]&0xF0) + ff_index;
		ff_index =  (++ff_index)%16;
	}
	if(pid == 0)
	{
		buf[3] = (buf[3]&0xF0) + pat_index;
		pat_index =  (++pat_index)%16;
	}
	if(pid == st->pmt_pid)
	{
		buf[3] = (buf[3]&0xF0) + pmt_index;
		pmt_index =  (++pmt_index)%16;
	}
	if(pid == st->video_pid)
	{
		is_video = 1;
		if(1 == ((buf[1]>>6)&0x01))
		{
			st->segment_time += 1.0/st->frame_rate_v;
			//printf("delta segment time is %lf\n", st->segment_time - st->prev_segment_time);
			is_frame_start = 1;
			is_key_frame = isKeyFrame(buf);
#if PRINT_LOG
			frame_num++;
			fprintf(fp_log, "Current TS segment file index is: %d\t", st->ts_file_index);
			fprintf(fp_log, "Current frame number is: %d\t",frame_num);
			if(is_key_frame)
				fprintf(fp_log, "this frame is a key frame\t");
			else
				fprintf(fp_log, "this frame is not a key fraeme\t");
			fprintf(fp_log, "current segment time is %f\n",(st->segment_time-st->prev_segment_time));
#endif
			if(is_key_frame && (st->segment_time - st->prev_segment_time)>= (st->segment_duration-0.5))
			{
				fclose(st->live_file_pointer);
				updateLiveM3u8File(livem3u8, st->ts_file_index, (st->segment_time-st->prev_segment_time));
				st->ts_file_index++;
				openTSFile(st->ts_file_index, st->ts_file_index,  st);
				st->prev_segment_time = st->segment_time;
			}
		}
	}
	fwrite(buf,1,TS_PACKET, st->live_file_pointer);
	fflush(st->live_file_pointer);
}

int isKeyFrame(uint8_t* buf)
{
	if(0==((buf[3]>>4)&0x01))
		return 0;

	if(2==((buf[3]>>4)&0x02))
	{
		int adaption_length = buf[4];
		for(int i=5+adaption_length;i<184;i++)
		{
			if((buf[i]==0x00)&&(buf[i+1]==0x00)&&(buf[i+2]==0x00)&&(buf[i+3]==0x01)&&(buf[i+4]==0x09))
			{
				if(buf[i+10]==0x67)
					return 1;
				else
					return 0;
			}
		}
	}
	else
	{
		for(int i=4;i<184;i++)
		{
			if((buf[i]==0x00)&&(buf[i+1]==0x00)&&(buf[i+2]==0x00)&&(buf[i+3]==0x01)&&(buf[i+4]==0x09))
			{
				if(buf[i+10]==0x67)
					return 1;
				else
					return 0;
			}
		}
	}
	return 0;
}

int openTSFile(int live_index, int ondemand_index, stream* st)
{
	//char *s=(char*)malloc(strlen(st->live_url)+15);
	char s[50];
	sprintf(s,"%s/%s%d.ts",st->live_url,st->prefix,live_index);
	st->live_file_pointer = fopen(s, "wb");
	//printf("S %s\n",s);
	//free(s);
	//s = (char*)malloc(strlen(st->ondemand_url)+15);
	if(st->live_file_pointer == NULL)
	{
		printf("open live ts file %d error\n", live_index);
		return 0;
	}
	st->extra_data[3] = (st->extra_data[3]&0xF0) + ff_index;
	st->extra_data[191] = (st->extra_data[191]&0xF0) + pat_index;
	st->extra_data[379] = (st->extra_data[379]&0xF0) + pmt_index;
	ff_index =  (++ff_index)%16;
	pat_index =  (++pat_index)%16;
	pmt_index =  (++pmt_index)%16;

	fwrite(st->extra_data, 1, 3*TS_PACKET, st->live_file_pointer);
	fflush(st->live_file_pointer);

	return 1;
}

void setDefaultOption(option* opt, char* cap, int flag)
{
	opt->segment_duration = 2;
	opt->hls_list_size = 3;
	snprintf(opt->prefix, 8,"default");

	opt->live_url = cap;
	if(!flag)
	{
		strcat(opt->live_url, "/live");
	}
}

void initOption(option* opt, char** argv, int argc)
{
	if(argc==1)
		return;
	for(int i=1;i<argc;++i)
	{
		if(strcmp(argv[i],"-hls_list_size")==0){i++;opt->hls_list_size = atoi(argv[i]);}
		if(strcmp(argv[i],"-segment_duration")==0){i++;opt->segment_duration = atof(argv[i]);}
		if(strcmp(argv[i],"-live_url")==0){i++;opt->live_url = argv[i];}
		if(strcmp(argv[i],"-ondemand_url")==0){i++;opt->ondemand_url = argv[i];}
		if(strcmp(argv[i],"-prefix")==0){++i; snprintf(opt->prefix, strlen(argv[i]), "%s", argv[i]);}
	}
	return;
}

void printUsage()
{
	printf("\n================================================Usage===========================================\n");
	printf("\t-hls_list_size(int): set the segment number of playlist entries. Default value is 3\n");
	printf("\t-segment_duration(float): set the segment length in seconds. Default value is 2.0\n");
	printf("\t-live_url: set the absolute address of segments. Default value is the current absolute path/live\n");
	printf("\t-prefix: set the prefix of segment and m3u8 file. Default value is \"default\"\n");
	printf("\n================================================Usage===========================================\n");
}
