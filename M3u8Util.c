#include "M3u8Util.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define MY_DEBUG
#ifdef MY_DEBUG
#define DEBUG_INFO(s) printf("%s\n", s)
#else
#define DEBUG_INFO(s)
#endif

#define TS_DURATION_TEMPLATE "#EXTINF:%3f,\n"
#define TS_FILE_NAME_TEMPLATE "%s%d.ts\n"
#define LIVE_M3U8_HEADER_TEMPLATE   "#EXTM3U8\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:%d\n#EXT-X-MEDIA-SEQUENCE:1\n"
#define ONDEMAND_M3U8_END_TEMPLATE "#EXT-X-ENDLIST\n"
#define HOST_ADDRESS "http://live.imoocs.org/"
//#define snprintf _snprintf
LiveM3u8* createLiveM3u8(uint8_t tsNum)
{
	LiveM3u8* ret = (LiveM3u8*)malloc(sizeof(LiveM3u8));
	ret->tsNum = tsNum;
	ret->entry = (TsEntry**)malloc(tsNum*sizeof(TsEntry*));
	for(int i = 0; i < tsNum; ++i)
	{
		ret->entry[i] = (TsEntry*)malloc(sizeof(TsEntry));
	}
	return ret;
}

void writeToFile(LiveM3u8* m3u8)
{
	//snprintf(m3u8->liveM3u8, ENTRY_LENGTH, "%s", "test.m3u8");
	FILE* fp;
	if((fp = fopen(m3u8->liveM3u8, "w+")) == NULL)
	{
		printf("Can't open %s\n", m3u8->liveM3u8);
		return;
	}
	fwrite(m3u8->header, strlen(m3u8->header), 1, fp);

	for(int i = 0; i < m3u8->tsNum; ++i)
	{
		fwrite(m3u8->entry[i]->duration, strlen(m3u8->entry[i]->duration), 1, fp);
		fwrite(m3u8->entry[i]->tsFile, strlen(m3u8->entry[i]->tsFile), 1, fp);
	}
	fclose(fp);

	if((fp = fopen(m3u8->onDemandM3u8, "a")) == NULL)
	{
		printf("Can't open %s\n", m3u8->onDemandPath);
		return;
	}
	fwrite(m3u8->onDemandEntry.duration, strlen(m3u8->onDemandEntry.duration), 1, fp);
	fwrite(m3u8->onDemandEntry.tsFile, strlen(m3u8->onDemandEntry.tsFile), 1, fp);
	fclose(fp);
}

void initLiveM3u8(LiveM3u8* m3u8, uint8_t maxDuration, const char* prefix, const char* path, char*  onDemandPath)
{
	snprintf(m3u8->header, HEADER_LENGTH, LIVE_M3U8_HEADER_TEMPLATE, maxDuration);
	snprintf(m3u8->liveM3u8,  ENTRY_LENGTH, "%s/%s.m3u8", path, prefix);
	snprintf(m3u8->tsPrefix, ENTRY_LENGTH, "%s%s/%s", HOST_ADDRESS, path, prefix);
	snprintf(m3u8->onDemandM3u8, ENTRY_LENGTH, "%s/%s.m3u8", onDemandPath, prefix);
	snprintf(m3u8->tsOnDemandPrefix, ENTRY_LENGTH, "%s/%s", onDemandPath, prefix);
	m3u8->onDemandIndex = 0;

	for(int i= 0; i < m3u8->tsNum; ++i)
	{
		TsEntry* entry = m3u8->entry[i];
		snprintf(entry->duration, ENTRY_LENGTH, TS_DURATION_TEMPLATE, maxDuration);
		snprintf(entry->tsFile, ENTRY_LENGTH, TS_FILE_NAME_TEMPLATE, m3u8->tsPrefix, i);
	}

	//write the onDemand header
	FILE* fp = NULL;
	if((fp = fopen(m3u8->onDemandM3u8, "w")) == NULL)
	{
		printf("Can't open %s\n", m3u8->onDemandM3u8);
		return;
	}
	fwrite(m3u8->header, strlen(m3u8->header), 1, fp);
	fclose(fp);

	writeToFile(m3u8);
}

void updateLiveM3u8File(LiveM3u8* m3u8, int index, double maxDuration)
{
	TsEntry* entry = m3u8->entry[index];
	snprintf(entry->duration, ENTRY_LENGTH, TS_DURATION_TEMPLATE, maxDuration);
	snprintf(entry->tsFile, ENTRY_LENGTH, TS_FILE_NAME_TEMPLATE, m3u8->tsPrefix, index);
	snprintf(m3u8->onDemandEntry.duration, ENTRY_LENGTH, TS_DURATION_TEMPLATE, maxDuration);
	snprintf(m3u8->onDemandEntry.tsFile, ENTRY_LENGTH, TS_FILE_NAME_TEMPLATE, m3u8->tsOnDemandPrefix, m3u8->onDemandIndex);

	writeToFile(m3u8);
	++m3u8->onDemandIndex;
}

void destroy(LiveM3u8* m3u8Ptr)
{
	//write to end should be moved out
	FILE* fp = NULL;
	if((fp = fopen(m3u8Ptr->onDemandM3u8, "a")) == NULL)
	{
		printf("Can't open %s\n", m3u8Ptr->onDemandPath);
		return;
	}
	fwrite(ONDEMAND_M3U8_END_TEMPLATE, strlen(ONDEMAND_M3U8_END_TEMPLATE), 1, fp);
	fclose(fp);
	for(int i = 0; i < m3u8Ptr->tsNum; ++i)
	{
		free(m3u8Ptr->entry[i]);
	}
	free(m3u8Ptr->entry);
	free(m3u8Ptr);
}
