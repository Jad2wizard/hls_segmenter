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
	pthread_mutex_lock(&db->dbLock);

	for(int i = 0; i < MAX_USER_NUM; ++i)
	{
		if(db->userEntries[i] == NULL)
		{
			db->userEntries[i] = malloc(sizeof(userAccount));
			db->userEntries[i]->fd = fd;
			snprintf(db->userEntries[i]->userName, MAX_USER_NAME_LENGTH + 1,"%s", name);
			pthread_mutex_unlock(&db->dbLock);
			return 0;
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
				pthread_mutex_unlock(&db->dbLock);
				return 0;
			}
		}
	}

	pthread_mutex_unlock(&db->dbLock);
	return 1;
}


