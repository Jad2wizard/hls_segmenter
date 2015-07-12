#include "segmenter.h"
int main(option* opt, LiveM3u8* livem3u8)
{
	option opt;
	opt.input_filename = "channel.ts";
	opt.input_file = fopen(opt.input_filename,"rb");
	opt.live_url = "./live";
	opt.ondemand_url = "./ondemand";
	opt.prefix = "test";
	opt.segment_duration = 5;
	opt.hls_list_size = 5;
	
	uint8_t* extra_data = (uint8_t*)malloc(188*3);
	fread(extra_data, 1, 188*3, opt.input_file);
	fseek(opt.input_file, 0, SEEK_END);
	int file_size = ftell(opt.input_file);
	fseek(opt.input_file, 0, SEEK_SET);

	uint8_t* buffer = (uint8_t*)malloc(188);
	stream st;
	st.segment_duration = opt.segment_duration;
	st.hls_list_size = opt.hls_list_size;
	st.getPID=0;
	st.pmt_pid=-1;
	st.video_pid=-1;
	st.audio_pid=-1;
	st.frame_rate_v = 30;
	st.frame_rate_a = 20;
	st.ts_file_index = 0;
	st.live_url = opt.live_url;
	st.ondemand_url = opt.ondemand_url;
	st.prefix = opt.prefix;
	st.extra_data = extra_data;	

	openTSFile(st.ts_file_index, 0, &st);
	LiveM3u8* livem3u8 = createLiveM3u8(st.hls_list_size);
	initLiveM3u8(livem3u8, st.segment_duration, st.prefix,st.live_url,st.ondemand_url);
	
	for(int i = 0; i<file_size/188; i++)
	{
		fread(buffer, 1, 188, opt.input_file);
		parseOneTS(buffer, &st, livem3u8);
	}

	printf("%d\t%d\t%d\n",st.pmt_pid,st.video_pid,st.audio_pid);
	return 0;
}

int parseOneTS(uint8_t* buf, stream* st, LiveM3u8* livem3u8)
{
	int pid =  ((buf[1]&0x1F)<<8|buf[2]);
	int is_key_frame = 0;
	int is_video = 0;
	int is_audio = 0;
	int is_frame_start = 0;
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
			if(buf[pos]==0x03)
			{
				st->audio_pid = ((buf[pos+1]<<8)|buf[pos+2])&0x1FFF;
				pos=pos+5;
				continue;
			}
			pos++;
		}
	}

	if(pid == st->video_pid)
	{
		is_video = 1;
		if(1 == ((buf[1]>>6)&0x01))
		{
			st->segment_time += 1.0/st->frame_rate_v;
			is_frame_start = 1;
			is_key_frame = isKeyFrame(buf);
			if(is_key_frame && (st->segment_time - st->prev_segment_time)>= (st->segment_duration-0.5))
			{
				fclose(st->live_file_pointer);
				fclose(st->ondemand_file_pointer);
				updateLiveM3u8File(livem3u8, st->ts_file_index%st->hls_list_size, (st->segment_time-st->prev_segment_time));
				st->ts_file_index++;
				openTSFile(st->ts_file_index%st->hls_list_size, st->ts_file_index,  st);
				
				st->prev_segment_time = st->segment_time;
			}
		}
	}
	fwrite(buf,1,188, st->live_file_pointer);
	fflush(st->live_file_pointer);
	fwrite(buf,1,188, st->ondemand_file_pointer);
	fflush(st->ondemand_file_pointer);
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
	return 0;
}

int openTSFile(int live_index, int ondemand_index, stream* st)
{
	//char *s=(char*)malloc(strlen(st->live_url)+15);
	char s[50];
	sprintf(s,"%s/%s%d.ts",st->live_url,st->prefix,live_index);
	st->live_file_pointer = fopen(s, "wb");
	//free(s);
	//s = (char*)malloc(strlen(st->ondemand_url)+15);
	if(st->live_file_pointer == NULL)
	{
		printf("open live ts file %d error\n", live_index);
		return 0;
	}
	fwrite(st->extra_data, 1, 3*188, st->live_file_pointer);
	fflush(st->live_file_pointer);	
	
	sprintf(s,"%s/%s%d.ts",st->ondemand_url,st->prefix,ondemand_index);
	st->ondemand_file_pointer = fopen(s, "wb");
	//free(s);
	if(st->ondemand_file_pointer == NULL)
	{
		printf("open ondemand ts file %d error\n", live_index);
		return 0;
	}
	fwrite(st->extra_data, 1, 3*188, st->ondemand_file_pointer);
	fflush(st->ondemand_file_pointer);
	
	return 1;
}

