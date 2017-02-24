//Program
//	a p2p chat with select
//History
//	Xinspace	2 APRIL	First release
//
/*
use:
	client:  $./a.out server_ip_addr server_port
	server:	 $./a.out server_port
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>  // for isdigit()
#include "jwHash.h"

#ifdef HASHTHREADED
#include <pthread.h>
#include <semaphore.h>
#endif

#define FUNW(s)\
{\
	fprintf(stderr, "%s is wrong!\n", (s));\
	exit(1);\
}

#define BUFSIZE 1024	//buf size
#define BACKLOG 10       // Number of allowed connections
#define NUMTHREADS 6
#define HASHCOUNT 100000

 typedef struct clientinfo {
    int fd;
    char* name;
    struct sockaddr_in addr; // client's address information
    char* data;
} clientinfo;

void *peer_talk(void *);	//client part in this program
void *server_talk(void *);	//server part in this program
//void process_cli( clientinfo *, int, char *, csHashTable *);  // process with client request
extern char *translate_result(const char *);    //refer to hash.c function for converting result

csHashTable * table = NULL;
// hash a million strings into various sizes of table
struct timeval tval_before, tval_done1, tval_done2, tval_done3;
struct timeval tval_writehash, tval_readhash, tval_deletehash;

typedef struct threadinfo {
    csHashTable *table;
    int start;
} threadinfo;

typedef struct procli_info {
    int socket;
    char* buffer;
    csHashTable *table;
} procli_info;

void * thread_func(void *arg)
{
	threadinfo *info = arg;
	char buffer[512];
	int i = info->start;
	csHashTable *table = info->table;
	free(info);
	for(;i<HASHCOUNT;i+=NUMTHREADS) {
		sprintf(buffer,"%d",i);
		add_int_by_str(table,buffer,i);
	}
}


static inline void timersub(
	const struct timeval *t1,
	const struct timeval *t2,
	struct timeval *res)
{
	res->tv_sec = t1->tv_sec - t2->tv_sec;
	if ( t1->tv_usec - t2->tv_usec < 0 ) {
		res->tv_sec--;
		res->tv_usec = t1->tv_usec - t2->tv_usec + 1000 * 1000;
	} else {
		res->tv_usec = t1->tv_usec - t2->tv_usec;
	}
}

int main(int argc, char **argv)
{
	if (3 == argc)	//argc == 3, instructing that this is client.
	{
		//pthread_t tidc;
		//pthread_create(&tidc, NULL, client_talk, (void *)argv);
		//pthread_join(tidc, NULL);
		return 0;
	}
	else if (2 == argc)	//argc == 2, instructing that this is server
	{
        // create
        //csHashTable * table = create_hash(10);
        table = create_hash(8);   //8 buckets per server
        if (!table) {
            // fail
            return 1;
        }

		pthread_t tids;
		pthread_create(&tids, NULL, server_talk, (void *)argv);
		pthread_join(tids, NULL);
	}

	printf("in main!\n");

	return 0;
}

void *peer_talk(void *arg)
{
	char **argu = (char **)arg;

	int connfd;
	struct sockaddr_in peer_to;
	int port;
	int maxfd;
	int n;
	int nbytes;
	int test = 0; // for testing
	char buf[BUFSIZE];
	fd_set rdfs, ordfs;

	if (-1 == (connfd = socket(PF_INET, SOCK_STREAM, 0)))
		FUNW("socket");

	//if (-1 == (port = atoi(argu[1]) + 1))
    if (-1 == (port = atoi("8889")))
		FUNW("atoi");

	peer_to.sin_family = PF_INET;
	peer_to.sin_port = htons(port);
	inet_aton("127.0.0.1", &(peer_to.sin_addr));
	//inet_aton(argu[1], &(server.sin_addr));

	if (-1 == connect(connfd, (struct sockaddr *)(&peer_to), sizeof(peer_to)))
		FUNW("connect");

	printf("connected to host:%s port:%d\n", inet_ntoa(peer_to.sin_addr), htons(peer_to.sin_port));

	FD_ZERO(&rdfs);
	FD_ZERO(&ordfs);
	FD_SET(connfd, &ordfs);
	FD_SET(STDIN_FILENO, &ordfs);
	maxfd = connfd;	//connfd is the best file descriptor which in the rdfs(read file set)

	while(1)
	{
		rdfs = ordfs;
		if (-1 == (n = select(maxfd + 1, &rdfs, NULL, NULL, NULL)))	//select function just monit read
		{
			FUNW("select");
		}
		else
		{
			if (FD_ISSET(connfd, &rdfs))	//if connfd is read-ready
			{
				if (-1 == (nbytes = read(connfd, buf, BUFSIZE)))
					FUNW("read");

				if (0 == strncasecmp(buf, "quit", 4))
				{
					printf("he quit!\npress any key to quit!\n");
					getchar();
					break;
				}
				//fprintf(stdout, "%s.%u:%s\n", inet_ntoa(peer_to.sin_addr), htons(peer_to.sin_port), translate_result(buf));
				fprintf(stdout, "%s\n", translate_result(buf));

				memset(buf, 0, sizeof(buf));
			}
			else if (FD_ISSET(STDIN_FILENO, &rdfs))	//if stdin is read-ready
			{
				if (-1 == (nbytes = read(STDIN_FILENO, buf, BUFSIZE)))
					FUNW("read");

				if (0 == strncasecmp(buf, "quit", 4))
				{
					printf("you quit!\npress any key to quit!\n");
					getchar();
					sprintf(buf, "quit!\n");
					if ( -1 == write(connfd, buf, strlen(buf)))
                        FUNW("write");
					break;
				}

				buf[--nbytes] = 0;


                char * str_put = "put ";
                char * str_get = "get ";
                char * str_del = "del ";
                int key, result;
                char str_i[256] ={0};
                if (!test)
                {
                    for (key = 100000; key <= 200000; ++key)
                    {
                        // buf = "put 100000 string100000"
                        test++;
                        strcpy(buf, str_put);
                        sprintf(str_i, "%d", key);
                        strncat(buf, str_i, strlen(str_i));
                        strcat(buf, " ");
                        strcat(buf, "string");
                        strncat(buf, str_i, strlen(str_i));
                        buf[strlen(buf)] = '\0';
                        //printf("buf:%s\n", buf);

                        if (-1 == (nbytes = write(connfd, buf, strlen(buf))))
                            FUNW("write");
                    }
                }
                else
                {
                        if (-1 == (nbytes = write(connfd, buf, strlen(buf))))
                            FUNW("write");
                }
/*
    printf("table.buckets = %d\n", (int)hashtable->bucketsinitial);
    getchar();
    char * str = "string ";
    char str_value[1000] = {0};
    int key, result;
    char str_i[256] ={0};
    gettimeofday(&tval_before, NULL);
    for (key = 100000; key <= 200000; ++key)
    {
        strcpy(str_value, str);
        sprintf(str_i, "%d", key);
        strcat(str_value, str_i);
       // printf("%s\n", str_value);
        add_str_by_int(hashtable, key, str_value);
    }
    gettimeofday(&tval_done1, NULL);
    char * str_getvalue;
    for (key = 100000; key <= 200000; ++key)
    {
        get_str_by_int(hashtable, key, &str_getvalue);
        //printf("get key=%d string=%s\n", key, str_getvalue);
    }
    gettimeofday(&tval_done2, NULL);
    for (key = 100000; key <= 200000; ++key)
    {
        result = del_by_int(hashtable, key);
        //printf("delete key=%d result=%d\n", key, result);
    }
    gettimeofday(&tval_done3, NULL);
	timersub(&tval_done1, &tval_before, &tval_writehash);
	timersub(&tval_done2, &tval_done1, &tval_readhash);
	timersub(&tval_done3, &tval_done2, &tval_deletehash);
    printf("%s.%u:%s\n", inet_ntoa((client->addr).sin_addr), htons((client->addr).sin_port), recvbuf);
    printf("Store 100000 strings by int: %ld.%06ld sec, read: %ld.%06ld sec, delete: %ld.%06ld sec\n",
		(long int)tval_writehash.tv_sec, (long int)tval_writehash.tv_usec,
		(long int)tval_readhash.tv_sec, (long int)tval_readhash.tv_usec,
		(long int)tval_deletehash.tv_sec, (long int)tval_deletehash.tv_usec);

    int t;
	pthread_t * threads[NUMTHREADS];
    gettimeofday(&tval_before, NULL);
	for(t=0;t<NUMTHREADS;++t) {
		pthread_t * pth = malloc(sizeof(pthread_t));
		threads[t] = pth;
		threadinfo *info = (threadinfo*)malloc(sizeof(threadinfo));
		info->table = hashtable; info->start = t;
		pthread_create(pth,NULL,thread_func,info);
	}
	for(t=0;t<NUMTHREADS;++t) {
		pthread_join(*threads[t], NULL);
	}
	gettimeofday(&tval_done1, NULL);
	int i,j;
	int error = 0;
	char buffer[512];
	for(i=0;i<HASHCOUNT;++i) {
		sprintf(buffer,"%d",i);
		get_int_by_str(hashtable,buffer,&j);
		if (i!=j) {
			printf("Error: %d != %d\n",i,j);
			error = 1;
		}
	}
	if (!error) {
		printf("No errors.\n");
	}
	gettimeofday(&tval_done2, NULL);
    for(i=0;i<HASHCOUNT;++i) {
		del_by_int(hashtable,i);
	}
	gettimeofday(&tval_done3, NULL);
	timersub(&tval_done1, &tval_before, &tval_writehash);
	timersub(&tval_done2, &tval_done1, &tval_readhash);
	timersub(&tval_done3, &tval_done2, &tval_deletehash);
    printf("%s.%u:%s\n", inet_ntoa((client->addr).sin_addr), htons((client->addr).sin_port), recvbuf);
    printf("Store 100000 strings by int: %ld.%06ld sec, read: %ld.%06ld sec, delete: %ld.%06ld sec\n",
		(long int)tval_writehash.tv_sec, (long int)tval_writehash.tv_usec,
		(long int)tval_readhash.tv_sec, (long int)tval_readhash.tv_usec,
		(long int)tval_deletehash.tv_sec, (long int)tval_deletehash.tv_usec);
*/
				//printf("I:%s\n", buf);

				memset(buf, 0, sizeof(buf));
			}
		}
	}

	close(connfd);

	return ((void *)NULL);
}

//void process_cli(clientinfo *client, int socketfd, char* recvbuf, csHashTable *hashtable)
void * process_cli(void *arg)
{
    procli_info *info = arg;
    char str_opr[10] = {0};
    char str_key[256] = {0};
    int result;
    char str_result[1000] = {0};
    if (0 == strncasecmp(info->buffer, "put", 3))
    {
        char str_value[718] ={0};
        sscanf(info->buffer, "%[^ ] %[^ ] %s", str_opr, str_key, str_value);
		//printf("value %s\n", str_value);
        long i_key = atoi(str_key);
        result = add_str_by_int(info->table, i_key, str_value);
        sprintf(str_result, "%d", result);
        if (-1 == write(info->socket, str_result, strlen(str_result)))
            FUNW("write");
		memset(str_result, 0, sizeof(str_result));
    }
	else if (0 == strncasecmp(info->buffer, "get", 3))
	{
		sscanf(info->buffer, "%[^ ] %[^ ]", str_opr, str_key);
		//printf("opr %s key %s\n", str_opr, str_key);
		long i_key = atoi(str_key);
        char * str_value;
        result = get_str_by_int(info->table, i_key, &str_value);
        if (HASHOK != result )
        {
            sprintf(str_result, "%d", result);
            if (-1 == write(info->socket, str_result, strlen(str_value)))
                FUNW("write");
            memset(str_result, 0, sizeof(str_result));
        }
        else
        {
            if (-1 == write(info->socket, str_value, strlen(str_value)))
                FUNW("write");
            memset(str_value, 0, sizeof(str_value));
        }
	}
	else if (0 == strncasecmp(info->buffer, "del", 3))
	{
		sscanf(info->buffer, "%[^ ] %[^ ]", str_opr, str_key);
		//printf("opr %s key %s\n", str_opr, str_key);
		long i_key = atoi(str_key);
        char * str_value;
        result = del_by_int(info->table, i_key);
        sprintf(str_result, "%d", result);
        //printf("%s\n", str_result);
		if (-1 == write(info->socket, str_result, strlen(str_result)))
            FUNW("write");
		memset(str_result, 0, sizeof(str_result));
	}
	free(info);
    /*
    gettimeofday(&tval_done2, NULL);
    for (key = 100000; key <= 200000; ++key)
    {
        result = del_by_int(hashtable, key);
        //printf("delete key=%d result=%d\n", key, result);
    }
    gettimeofday(&tval_done3, NULL);
	timersub(&tval_done1, &tval_before, &tval_writehash);
	timersub(&tval_done2, &tval_done1, &tval_readhash);
	timersub(&tval_done3, &tval_done2, &tval_deletehash);
    printf("%s.%u:%s\n", inet_ntoa((client->addr).sin_addr), htons((client->addr).sin_port), recvbuf);
    printf("Store 100000 strings by int: %ld.%06ld sec, read: %ld.%06ld sec, delete: %ld.%06ld sec\n",
		(long int)tval_writehash.tv_sec, (long int)tval_writehash.tv_usec,
		(long int)tval_readhash.tv_sec, (long int)tval_readhash.tv_usec,
		(long int)tval_deletehash.tv_sec, (long int)tval_deletehash.tv_usec);

    int t;
	pthread_t * threads[NUMTHREADS];
    gettimeofday(&tval_before, NULL);
	for(t=0;t<NUMTHREADS;++t) {
		pthread_t * pth = malloc(sizeof(pthread_t));
		threads[t] = pth;
		threadinfo *info = (threadinfo*)malloc(sizeof(threadinfo));
		info->table = hashtable; info->start = t;
		pthread_create(pth,NULL,thread_func,info);
	}
	for(t=0;t<NUMTHREADS;++t) {
		pthread_join(*threads[t], NULL);
	}
	gettimeofday(&tval_done1, NULL);
	int i,j;
	int error = 0;
	char buffer[512];
	for(i=0;i<HASHCOUNT;++i) {
		sprintf(buffer,"%d",i);
		get_int_by_str(hashtable,buffer,&j);
		if (i!=j) {
			printf("Error: %d != %d\n",i,j);
			error = 1;
		}
	}
	if (!error) {
		printf("No errors.\n");
	}
	gettimeofday(&tval_done2, NULL);
    for(i=0;i<HASHCOUNT;++i) {
		del_by_int(hashtable,i);
	}
	gettimeofday(&tval_done3, NULL);
	timersub(&tval_done1, &tval_before, &tval_writehash);
	timersub(&tval_done2, &tval_done1, &tval_readhash);
	timersub(&tval_done3, &tval_done2, &tval_deletehash);
    printf("%s.%u:%s\n", inet_ntoa((client->addr).sin_addr), htons((client->addr).sin_port), recvbuf);
    printf("Store 100000 strings by int: %ld.%06ld sec, read: %ld.%06ld sec, delete: %ld.%06ld sec\n",
		(long int)tval_writehash.tv_sec, (long int)tval_writehash.tv_usec,
		(long int)tval_readhash.tv_sec, (long int)tval_readhash.tv_usec,
		(long int)tval_deletehash.tv_sec, (long int)tval_deletehash.tv_usec);
    */
}

void *server_talk(void *arg)
{
	char **argu = (char **)arg;

	int listenfd, connectfd;    //socket information
	int maxfd;
	int port;
	int i, maxi, sockfd;
	int nready;
	//ssize_t n;
	int nbytes;
	struct sockaddr_in server;  // server's address information
	clientinfo client[FD_SETSIZE];
	char buf[BUFSIZE];
	socklen_t addrlen;
	fd_set rdfs, allfs;

    /* create TCP socket */
	if (-1 == (listenfd = socket(PF_INET, SOCK_STREAM, 0)))
		FUNW("socket");

    int opt = SO_REUSEADDR;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	bzero(&server, sizeof(server));
	if (-1 == (port = atoi(argu[1])))    // get server port
		FUNW("atoi");
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (-1 == bind(listenfd, (struct sockaddr *)(&server), sizeof(server)))
		FUNW("bind");

	if (-1 == listen(listenfd, BACKLOG))
		FUNW("listen");

	addrlen = sizeof(struct sockaddr_in);

    /* initialize for select */
	maxfd = listenfd;
	maxi = -1;
	for (i = 0; i < FD_SETSIZE; i++)
	{
        client[i].fd = -1;
	}
    FD_ZERO(&rdfs);
	FD_ZERO(&allfs);

	FD_SET(STDIN_FILENO, &allfs);
	FD_SET(listenfd, &allfs);
	while(1)
	{
        struct sockaddr_in addr;
		rdfs = allfs;
		if (0 > (nready = select(maxfd + 1, &rdfs, NULL, NULL, NULL)))	//same as peer_talk function's select
		{
			FUNW("select");
		}
		else
		{
			if (FD_ISSET(listenfd, &rdfs))	//if listenfd is read_ready, we can accept new connection
			{
                /* accept connection */
				if (-1 == (connectfd = accept(listenfd, (struct sockaddr *)(&addr), &addrlen)))
				{
					FUNW("accept");
				}
                /* put new fd to client */
                for (i = 0; i < FD_SETSIZE; i++)
                    if (client[i].fd < 0)
                    {
                        client[i].fd = connectfd;   // save descriptor
                        client[i].name = (unsigned char *)malloc(BUFSIZE);  //update for Segment Fault(core dumped)
                        client[i].name[0] = '\0';
                        memcpy((struct sockaddr *)&(client[i].addr), &addr, sizeof(struct sockaddr));
                        client[i].data = (unsigned char *)malloc(BUFSIZE);
                        client[i].data[0] = '\0';
                        printf("connected from host:%s port:%u\n", inet_ntoa(client[i].addr.sin_addr), htons(client[i].addr.sin_port));
                        break;
                    }
                if (i == FD_SETSIZE)
                    printf("too many clients.\n");
                FD_SET(connectfd, &allfs);  // add new connect descriptor to set
                maxfd = maxfd > connectfd ? maxfd : connectfd;	//if connectfd is greater than maxfd, then change them
                if (i > maxi)   maxi = i;
                if (--nready <= 0)  continue;   // no more readable descriptors
            }
			else if (FD_ISSET(STDIN_FILENO, &rdfs))
			{
				if (0 > (nbytes = read(STDIN_FILENO, buf, sizeof(buf))))
					FUNW("read");

				if (0 == strncasecmp(buf, "quit", 4))
				{
					printf("you quit!\npress any key to quit!\n");
					getchar();
					sprintf(buf, "quit!\n");
					if (0 > write(connectfd, buf, strlen(buf)))
                        FUNW("write");
					break;
				}
				else if (0 == strncasecmp(buf, "connect", 7))
				{
                    char s_IP[16] = {0};
                    char s_port[6] = {0};
                    char s_command[5] = {0};
                    sscanf(buf, "%[^ ] %[^ ] %[^ ]", s_command, s_IP, s_port);
                    printf("IP:Port:%s,%s,%s\n", s_command, s_IP, s_port);
                    //getchar();
                    pthread_t tidc;
                    pthread_create(&tidc, NULL, peer_talk, (void *)argu);
                    pthread_join(tidc, NULL);
				}
				else if (0 == strncasecmp(buf, "initialize", 10))
				{
					FILE *fconfig;
					fconfig = fopen("./server.cfg", "r");
					if(fconfig == NULL)
    				{
        				perror("open config file error");
        				return -1;
    				}

					fclose(fconfig);

				}
				buf[--nbytes] = 0;

				//printf("I:%s\n", buf);

				//if (0 > (nbytes = write(STDOUT_FILENO, buf, strlen(buf))))
				//	FUNW("write");

				memset(buf, 0, sizeof(buf));
				buf[0] = '\0';
			}

            //printf("maxi =%d\n", maxi);
            for (i = 0; i <= maxi; i++)     // check all clients for data
            {
                if ((sockfd = client[i].fd) < 0)    continue;
                if (FD_ISSET(sockfd, &rdfs))
                {
                    nbytes = read(sockfd, buf, sizeof(buf));
                    //printf("bytes=%d\n", nbytes);
                    if (0 > nbytes)
                    {
                        FUNW("read");
                    }
                    else
                    {
                        if ( (0 == nbytes) || (0 == strncasecmp(buf, "quit", 4)) )
                        {
                            // connect closed by client
                            fprintf(stdout, "he quit!\n");
                            close(sockfd);
                            printf("Client:%s closed connection. User's data:%s\n", client[i].name, client[i].data);
                            FD_CLR(sockfd, &allfs);
                            client[i].fd = -1;
                            bzero(client[i].name, BUFSIZE);
                            bzero(client[i].data, BUFSIZE);
                        }
                        else
                        {
                            buf[nbytes] = '\0'; // read nbytes bytes
                            pthread_t * pth = malloc(sizeof(pthread_t));
                            procli_info *info = (procli_info*)malloc(sizeof(procli_info));
                            info->table = table; info->buffer = buf; info->socket = sockfd;
                            pthread_create(pth, NULL, process_cli, info);
                            pthread_join(*pth, NULL);
                            //process_cli(&client[i], sockfd, buf, table);
                        }
                        if (--nready <= 0)   break; /* no more readable descriptors */
                    }
                }
            }
		}
	}
	close(listenfd);
	return ((void *)NULL);
}
