#ifndef USER_CERTIFICATION
#define USER_CERTIFICATION

#include <unistd.h>  
#include <pthread.h> 
#include <stdlib.h>

#define MAX_USER_NUM 150
#define MAX_USER_NAME_LENGTH 50

typedef struct
{
	char userName[MAX_USER_NAME_LENGTH + 1];
	int fd;
	//the next two member is used to accomplish the goal that when the client reconnecting with the same id, the segNum will continue instad become to 0
	int live;//0 not live; 1 live
	int segNum;
} userAccount;


//just use array for easy implementation
typedef struct
{
	userAccount* userEntries[MAX_USER_NUM];
	pthread_mutex_t dbLock;
	char clientNum;
} accountDb;


extern accountDb userDb;

void initAccountDb(accountDb* db);
void destroyAccountDb(accountDb* db);


userAccount* findAvailableSlot(accountDb* db, char* name, int fd);


int deleteFromDb(accountDb* db, const char* name, int segNum);

char getClientNum(accountDb* db);
#endif
