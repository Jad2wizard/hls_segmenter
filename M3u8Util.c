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

	// init the cycle list
	ret->oldEntry = (TsEntry*)malloc(sizeof(TsEntry));
	TsEntry* previous = ret->oldEntry;
	for(int i = 1; i < tsNum; ++i)
	{
		TsEntry* current = (TsEntry*)malloc(sizeof(TsEntry));
		previous->next = current;
		current->prev = previous;
		previous = current;
	}
	previous->next = ret->oldEntry;
	ret->oldEntry->prev = previous;

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

	TsEntry* entry = m3u8->oldEntry;
	do
	{
		fwrite(entry->duration, strlen(entry->duration), 1, fp);
		fwrite(entry->tsFile, strlen(entry->tsFile), 1, fp);
		entry = entry->next;
	}while(entry != m3u8->oldEntry);
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
	//onDemandPath is not used any more
	snprintf(m3u8->header, HEADER_LENGTH, LIVE_M3U8_HEADER_TEMPLATE, maxDuration);
	snprintf(m3u8->liveM3u8,  ENTRY_LENGTH, "%s/%s.m3u8", path, prefix);
	snprintf(m3u8->tsPrefix, ENTRY_LENGTH, "%s%s/%s", HOST_ADDRESS, path, prefix);
	snprintf(m3u8->onDemandM3u8, ENTRY_LENGTH, "%s/%sDemand.m3u8", path, prefix);
	snprintf(m3u8->tsOnDemandPrefix, ENTRY_LENGTH, "%s%s/%s", HOST_ADDRESS, path, prefix);

	TsEntry* entry = m3u8->oldEntry;
	do
	{
		snprintf(entry->duration, ENTRY_LENGTH, TS_DURATION_TEMPLATE, maxDuration);
		snprintf(entry->tsFile, ENTRY_LENGTH, TS_FILE_NAME_TEMPLATE, m3u8->tsPrefix, 0);
		entry = entry->next;
	}while(entry != m3u8->oldEntry);

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
	TsEntry* entry = m3u8->oldEntry;
	snprintf(entry->duration, ENTRY_LENGTH, TS_DURATION_TEMPLATE, maxDuration);
	snprintf(entry->tsFile, ENTRY_LENGTH, TS_FILE_NAME_TEMPLATE, m3u8->tsPrefix, index);
	snprintf(m3u8->onDemandEntry.duration, ENTRY_LENGTH, TS_DURATION_TEMPLATE, maxDuration);
	snprintf(m3u8->onDemandEntry.tsFile, ENTRY_LENGTH, TS_FILE_NAME_TEMPLATE, m3u8->tsOnDemandPrefix, index);
	m3u8->oldEntry = m3u8->oldEntry->next;
	writeToFile(m3u8);
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
	
	//free cycle list
	TsEntry* current = m3u8Ptr->oldEntry;
	do
	{
		TsEntry* next = current->next;
		free(current);
		current = next;
	}while(current != m3u8Ptr->oldEntry);
	free(m3u8Ptr);
}
