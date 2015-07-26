#ifndef SEGMENTERUTIL_H
#define SEGMENTERUTIL_H
#include <stdint.h>

#define ENTRY_LENGTH 50
#define HEADER_LENGTH 100
typedef struct TsEntry
{
	char duration[ENTRY_LENGTH];
	char tsFile[ENTRY_LENGTH];
	struct TsEntry* prev;
	struct TsEntry* next;
} TsEntry;
typedef struct
{
	int8_t tsNum;
	uint8_t maxDuration;
	TsEntry* oldEntry;//it points to the oldest ts file in a cycle list
	char header[HEADER_LENGTH];
	char liveM3u8[ENTRY_LENGTH];
	char tsPrefix[ENTRY_LENGTH];
	TsEntry onDemandEntry;
	char onDemandPath[ENTRY_LENGTH];
	char onDemandM3u8[ENTRY_LENGTH];
	char tsOnDemandPrefix[ENTRY_LENGTH];
} LiveM3u8;


LiveM3u8* createLiveM3u8(uint8_t tsNum);
void initLiveM3u8(LiveM3u8* m3u8, uint8_t maxDuration, const char* prefix, const char* path, char* onDemandPath);
void updateLiveM3u8File(LiveM3u8* m3u8, int index, double duration);
void destroy(LiveM3u8* m3u8Ptr);
#endif
