#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include "segmenter.h"
#include "UserCertification.h"

int main(int argc, char** argv)
{
	//need to parse from arguments
	short port = 1234;
	char current_absolute_path[100];
	realpath("./", current_absolute_path);
	printf("%s\n",current_absolute_path);
	int listenfd, connfd;
	struct sockaddr_in servaddr;
	uint8_t* extra_data = (uint8_t*)malloc(3*188);
	FILE* fp = fopen("head","rb");
	fwrite(extra_data, 1, 3*188, fp);
	printUsage();
	
	initAccountDb(&userDb);
	
	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		fprintf(stderr, "create socket error:%s(errno: %d)\n",strerror(errno), errno);
		exit(0);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	if(bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) == -1)
	{
		fprintf(stderr, "bind socket errot:%s(errno: %d)\n",strerror(errno), errno);
		exit(0);
	}

	if(listen(listenfd, 10) == -1)
	{
		fprintf(stderr, "listen socket error: %s(error: %d)\n",strerror(errno), errno);
		exit(0);
	}

	while(1)
	{
		if((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1)
		{
			fprintf(stderr, "accept socket error:%s(errno: %d)\n",strerror(errno), errno);
			exit(0);
		}
		pthread_t pth;
	//	hlsSegmentInfo* info = malloc(sizeof(hlsSegmentInfo));
		option* opt = (option*)malloc(sizeof(option));
		setDefaultOption(opt, current_absolute_path);
		initOption(opt, argv, argc);	
		opt->extra_data = extra_data;
		opt->input_file = connfd;
		pthread_create(&pth, NULL, segmenter, (void*)opt);
	}

	destroyAccountDb(&userDb);

	return 0;
}
