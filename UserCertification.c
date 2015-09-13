#include "UserCertification.h"
#include <string.h>

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

//return 0, if succeed
//return 1, if db is full
//return 2, if name is duplicate
int findAvailableSlot(accountDb* db, char* name, int fd)
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
				pthread_mutex_unlock(&db->dbLock);
				return 2;
			}
		}
	}

	if(availableSlot != MAX_USER_NUM)
	{
		db->userEntries[availableSlot] = malloc(sizeof(userAccount));
		db->userEntries[availableSlot]->fd = fd;
		snprintf(db->userEntries[availableSlot]->userName, MAX_USER_NAME_LENGTH + 1,"%s", name);
		db->clientNum++;
		pthread_mutex_unlock(&db->dbLock);
		return 0;

	}

	pthread_mutex_unlock(&db->dbLock);
	return 1;
}

//0 succeed
//1, failed
int deleteFromDb(accountDb* db, const char* name)
{
	pthread_mutex_lock(&db->dbLock);
	for(int i = 0; i < MAX_USER_NUM; ++i)
	{
		if(db->userEntries[i] != NULL)
		{
			if(strcmp(name, db->userEntries[i]->userName) == 0)
			{
				free(db->userEntries[i]);
				db->userEntries[i] = NULL;
				pthread_mutex_unlock(&db->dbLock);
				return 0;
			}
		}
	}

	db->clientNum--;
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
