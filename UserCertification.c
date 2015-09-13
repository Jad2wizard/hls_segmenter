#include "UserCertification.h"
#include <string.h>
#include <stdio.h>

accountDb userDb;

void initAccountDb(accountDb* db)
{
	pthread_mutex_init(&db->dbLock, NULL);

	for(int i = 0; i < MAX_USER_NUM; ++i)
	{
		db->userEntries[i] = NULL;
	}
	db->clientNum = 0;
}

void destroyAccountDb(accountDb* db)
{
	pthread_mutex_destroy(&db->dbLock);
	for(int i = 0; i < MAX_USER_NUM; ++i)
	{
		if(db->userEntries[i] != NULL)
			free(db->userEntries[i]);
	}
}

//return NULL, failed
//,succeed
userAccount* findAvailableSlot(accountDb* db, char* name, int fd)
{
	int availableSlot = MAX_USER_NUM;	
	pthread_mutex_lock(&db->dbLock);

	for(int i = 0; i < MAX_USER_NUM; ++i)
	{
		if(db->userEntries[i] == NULL)
		{
			if(availableSlot == MAX_USER_NUM)
				availableSlot = i;
		}
		else
		{
			if(strcmp(name, db->userEntries[i]->userName) == 0)
			{
				if(db->userEntries[i]->live == 0)
				{
					db->userEntries[i]->live = 1;
					db->clientNum++;
					pthread_mutex_unlock(&db->dbLock);
					return db->userEntries[i];
				}
				else
				{
					pthread_mutex_unlock(&db->dbLock);
					return NULL;
				}
			}
		}
	}

	if(availableSlot != MAX_USER_NUM)
	{
		db->userEntries[availableSlot] = malloc(sizeof(userAccount));
		db->userEntries[availableSlot]->fd = fd;
		db->userEntries[availableSlot]->live = 1;
		db->userEntries[availableSlot]->segNum = 0;
		snprintf(db->userEntries[availableSlot]->userName, MAX_USER_NAME_LENGTH + 1,"%s", name);
		db->clientNum++;
		pthread_mutex_unlock(&db->dbLock);
		return db->userEntries[availableSlot];

	}

	pthread_mutex_unlock(&db->dbLock);
	printf("The client num is full\n");
	return NULL;
}

//0 succeed
//1, failed
int deleteFromDb(accountDb* db, const char* name, int segNum)
{
	pthread_mutex_lock(&db->dbLock);
	for(int i = 0; i < MAX_USER_NUM; ++i)
	{
		if(db->userEntries[i] != NULL)
		{
			if(strcmp(name, db->userEntries[i]->userName) == 0)
			{
				//free(db->userEntries[i]);
				//db->userEntries[i] = NULL;
				//set the id not alive instead of free it
				db->userEntries[i]->live = 0;
				//Is it necessary to segNum + 1, segNum should be the next ts file index. 
				db->userEntries[i]->segNum = segNum;
				db->clientNum--;
				pthread_mutex_unlock(&db->dbLock);
				return 0;
			}
		}
	}

	pthread_mutex_unlock(&db->dbLock);
	return 1;
}

char getClientNum(accountDb* db)
{
	char ret = 0;
	pthread_mutex_lock(&db->dbLock);
	ret = db->clientNum;
	pthread_mutex_unlock(&db->dbLock);

	return ret;
}
