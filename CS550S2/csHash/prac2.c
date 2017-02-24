//Program
//	a Distribitued Hash Table with select
//History
//  Oct-03-2015 Start
//  Oct-11-2015 End
//
/*
usage:	server:	 $./test server_port
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
#include <ifaddrs.h>    // for struct ifaddrs
#include "csHash.h"

#ifdef HASHTHREADED
#include <pthread.h>
#include <semaphore.h>
#endif

#define FUNW(s)\
{\
	fprintf(stderr, "%s is wrong!\n", (s));\
	exit(1);\
}

#define CONFIG_FILE     "servers.cfg"
#define BUFSIZE         1024    // buffer size
#define BACKLOG         10      // Number of allowed connections
#define NUMTHREADS      6
#define HASHCOUNT       100000

#define pr_console()    printf("> ")
#define pr_msg(messsge) printf("> %s\n", message)

typedef struct htpeer {
    int fd;
    char* name;
    struct sockaddr_in addr; // client's address information
    char* data;
} htpeer;

void *peer_talk(void *);	//client part in this program
void *server_talk(void *);	//server part in this program
//void process_cli( clientinfo *, int, char *, csHashTable *);  // process with client request

csHashTable * table = NULL;
// hash a million strings into various sizes of table
struct timeval tval_before, tval_done1, tval_done2, tval_done3, tval_done4, tval_done5;
struct timeval tval_writehash, tval_readhash, tval_deletehash;

#ifdef HASHTEST
typedef struct threadinfo {
    csHashTable *table;
    int start;
} threadinfo;
#endif // HASHTEST

typedef struct procli_info {
    int socket;
    char* buffer;
    csHashTable *table;
} procli_info;


// http://stackoverflow.com/a/12996028
// hash function for int keys
static inline long int hashInt(long int x)
{
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x);
	return x;
}

// helper for converting hash result into readable string
// without static for reference
static inline char* translate_result(char * result)
{
    if (isdigit(result[0]) && 1 == strlen(result))
    {
        switch (result[0])
        {
            // fixed: is char '0', not 0
            case '0':
                return "OK";
            case '1':
                return "Added";
            case '2':
                 return "Replaced Value";
            case '3':
                 return "Already Added";
            case '4':
                 return "Deleted";
            case '5':
                 return "Not Found";
            default:
                 return "OK";
        }
    }
    else
          return result;
}

#ifdef HASHTEST
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
#endif // HASHTEST

/* Read configuration file */
int read_cfg_file(htpeer ht_peers[], int *servers_no)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char s_ip[16] = {0};
    char s_port[5] = {0};
    int lines = 0;
    struct ifaddrs * ifalladdrs = NULL;
    void * tmpaddr = NULL;
    char s_allip[160] = {0};
    char addrbuff[INET_ADDRSTRLEN] = {0};

    if ((fp = fopen(CONFIG_FILE, "r")) == NULL)
    {
        printf("Error opening config file %s\n", CONFIG_FILE);
        return 1;
    }

    //find all ip addresses for localhost
    getifaddrs(&ifalladdrs);
    while (ifalladdrs != NULL)
    {
        // get all ipv4 ip address, omit supporting ipv6
        if (ifalladdrs->ifa_addr->sa_family == AF_INET)
        {
            tmpaddr = &((struct sockaddr_in *)ifalladdrs->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpaddr, addrbuff, INET_ADDRSTRLEN);
            strncat(s_allip, addrbuff, INET_ADDRSTRLEN);
        }
        ifalladdrs = ifalladdrs->ifa_next;
    }

    while ((read = getline(&line, &len, fp)) != -1)
    {
        if(lines > FD_SETSIZE)
        {
            printf("Too many hash table servers %d\n", lines);
            break;
        }
        line[strlen(line) - 1] = 0;
        sscanf(line, "%[^:]:%s", s_ip, s_port);
        //The server has the same IP, not support in requirement
        #ifdef HASHTEST
                if (0 != strstr(s_allip, s_ip))
                  continue;
        #endif
        // Do not forget initialize sin_family, will result in read error
        ht_peers[lines].addr.sin_family = AF_INET;
        inet_aton(s_ip, &(ht_peers[lines].addr.sin_addr));
        ht_peers[lines].addr.sin_port = htons(atoi(s_port));
        lines++;
    }
    *servers_no = lines;
    if (0 > lines)
        printf("No servers connected.\n");
    fclose(fp);
    if (line)
        free(line);
    if(ifalladdrs)
        free(ifalladdrs);
    return 0;
}

int main(int argc, char **argv)
{
    #ifdef HASHTEST
        printf("In TEST mode\n");
    #endif
    if (2 == argc)	//argc == 2, instructing that this is server
	{
        //initialize server's hashtable
        table = create_hash(HASHCOUNT>>4);   // 100000/16 buckets per server
        //table = create_hash(9973);    // 9973 is a prime number
        if (!table) {
            // fail
            return 1;
        }
        pthread_t tids;
		pthread_create(&tids, NULL, server_talk, (void *)argv);
		pthread_join(tids, NULL);
	}
	else
	{
        printf("Uasge: %s port\n", argv[0]);
	}
	return 0;
}

#ifdef HASHTEST
void *peer_talk(void *arg)
{
    char **argu = (char **)arg;
    int maxfd;
    int n;  //hash table servers cursor
    int ready_to;
    int nbytes;
    char buf[BUFSIZE];
    htpeer server[FD_SETSIZE];
    fd_set rdfs, ordfs;

    memset(server, 0, FD_SETSIZE*sizeof(htpeer));
    int serv_cnt;  // server counts
    read_cfg_file(server, &serv_cnt);

    struct sockaddr_in serv;
    struct sockaddr_in guest;
    socklen_t socklen = sizeof(serv);
    FD_ZERO(&rdfs);
    FD_ZERO(&ordfs);
    for (n=0; n<serv_cnt; n++)
    {
	    if (-1 == (server[n].fd = socket(AF_INET, SOCK_STREAM, 0)))
	    	FUNW("socket");
        if (-1 == connect(server[n].fd, (struct sockaddr *)(&(server[n].addr)), sizeof(struct sockaddr)))
            FUNW("connect");

        printf("connected to connectfd:%d host:%s port:%d\n", server[n].fd, inet_ntoa(server[n].addr.sin_addr), htons(server[n].addr.sin_port));
        //printf("host %s:%d, guest %s:%d\n", inet_ntoa(serv.sin_addr), htons(serv.sin_port), inet_ntoa(guest.sin_addr), htons(guest.sin_port));

        FD_SET(server[n].fd, &ordfs);
        //connfd is the best file descriptor which in the rdfs(read file set)
        maxfd = maxfd > server[n].fd ? maxfd : server[n].fd;
    }
    FD_SET(STDIN_FILENO, &ordfs);

	while(1)
	{
		rdfs = ordfs;
		if (-1 == (ready_to = select(maxfd + 1, &rdfs, NULL, NULL, NULL)))	//select function just monit read
		{
			FUNW("select");
		}
		else
		{
            if (FD_ISSET(STDIN_FILENO, &rdfs))	//if stdin is read-ready
			{
                char str_cmd[10] ={0};
                char str_key[256] ={0};
				if (-1 == (nbytes = read(STDIN_FILENO, buf, BUFSIZE)))
					FUNW("read1");

				if (0 == strncasecmp(buf, "quit", 4))
				{
					printf("you quit!\npress any key to quit!\n");
					getchar();
					sprintf(buf, "quit!\n");
					for (n=0; n<serv_cnt; n++)   // tell all servers you quit
					{
                        if ( -1 == (server[n].fd, buf, strlen(buf), 0))
                            FUNW("write");
					}
                    bzero(buf, BUFSIZE);
                    buf[0] = '\0';
					break;
				}
				else if (0 == strncasecmp(buf, "put", 3))
				{
                    sscanf(buf, "%[^ ] %s", str_cmd, str_key);
                    char * str_put = "put ";
                    long key, result;
                    key = atoi(str_key) * 100000;
                    result = key + 100000;
                    printf("start->");
                    getchar();
                    gettimeofday(&tval_before, NULL);
                    for (; key < result; key++)
                    {
                        //buf = "put 100000 string100000"
                        strcpy(buf, str_put);
                        sprintf(str_key, "%ld", key);
                        strncat(buf, str_key, strlen(str_key));
                        strcat(buf, " ");
                        strcat(buf, "string");
                        strncat(buf, str_key, strlen(str_key));
                        buf[strlen(buf)] = '\0';

                        //send to server[hash]
                        size_t hash = hashInt(atoi(str_key)) % serv_cnt;
                        if (-1 == (nbytes = write(server[hash].fd, buf, strlen(buf))))
                            FUNW("write");
                        bzero(buf, BUFSIZE);
                        buf[0] = '\0';
                        if (-1 == (nbytes = read(server[hash].fd, buf, BUFSIZE)))
                            FUNW("read2");
                        bzero(buf, BUFSIZE);
                        buf[0] = '\0';
                    }
                    gettimeofday(&tval_done1, NULL);
                    printf("done1\n");
				}
				else if (0 == strncasecmp(buf, "get", 3))
				{
                    sscanf(buf, "%[^ ] %s", str_cmd, str_key);
                    char * str_get = "get ";
                    long key, result;
                    key = atoi(str_key) * 100000;
                    result = key + 100000;
                    printf("start->");
                    getchar();
                    gettimeofday(&tval_done2, NULL);
                    for (; key < result; key++)
                    {
                        strcpy(buf, str_get);
                        sprintf(str_key, "%ld", key);
                        strncat(buf, str_key, strlen(str_key));
                        buf[strlen(buf)] = '\0';
                        size_t hash = hashInt(atoi(str_key)) % serv_cnt;
                        if (-1 == (nbytes = write(server[hash].fd, buf, strlen(buf))))
                            FUNW("write");
                        bzero(buf, BUFSIZE);
                        buf[0] = '\0';
                        if (-1 == (nbytes = read(server[hash].fd, buf, BUFSIZE)))
                            FUNW("read2");
                        bzero(buf, BUFSIZE);
                        buf[0] = '\0';
                    }
                    gettimeofday(&tval_done3, NULL);
                    printf("done2\n");
				}
				else if(0 == strncasecmp(buf, "del", 3))
				{
                    sscanf(buf, "%[^ ] %s", str_cmd, str_key);
                    char * str_del = "del ";
                    long key, result;
                    key = atoi(str_key) * 100000;
                    result = key + 100000;
                    printf("start->");
                    getchar();
                    gettimeofday(&tval_done4, NULL);
                    for (; key < result; key++)
                    {
                        strcpy(buf, str_del);
                        sprintf(str_key, "%ld", key);
                        strncat(buf, str_key, strlen(str_key));
                        buf[strlen(buf)] = '\0';
                        size_t hash = hashInt(atoi(str_key)) % serv_cnt;
                        if (-1 == (nbytes = write(server[hash].fd, buf, strlen(buf))))
                            FUNW("write");
                        bzero(buf, BUFSIZE);
                        buf[0] = '\0';
                        if (-1 == (nbytes = read(server[hash].fd, buf, BUFSIZE)))
                            FUNW("read2");
                        bzero(buf, BUFSIZE);
                        buf[0] = '\0';
                    }
                    gettimeofday(&tval_done5, NULL);
                    printf("done3\n");
				}
				else if (0 == strncasecmp(buf, "time", 4))
				{
                    timersub(&tval_done1, &tval_before, &tval_writehash);
                    timersub(&tval_done3, &tval_done2, &tval_readhash);
                    timersub(&tval_done5, &tval_done4, &tval_deletehash);
                    printf("Store 100000 strings by int for %d servers: %ld.%06ld sec, read: %ld.%06ld sec, delete: %ld.%06ld sec\n",
                    serv_cnt,
                    (long int)tval_writehash.tv_sec, (long int)tval_writehash.tv_usec,
                    (long int)tval_readhash.tv_sec, (long int)tval_readhash.tv_usec,
                    (long int)tval_deletehash.tv_sec, (long int)tval_deletehash.tv_usec);
				}
                if(--ready_to <= 0) continue;  // mo more readable descriptors
			}
		}   // end else select
	}   // end while
    for (n=0; n<serv_cnt; n++)
        close(server[n].fd);
	return ((void *)NULL);
}
#else
void *peer_talk(void *arg)
{
	char **argu = (char **)arg;
	int maxfd;
	int n;  //hash table servers cursor
    int ready_to;
	int nbytes;
	char buf[BUFSIZE];
    htpeer server[FD_SETSIZE];
	fd_set rdfs, ordfs;

    memset(server, 0, FD_SETSIZE*sizeof(htpeer));
    int serv_cnt;  // server counts
    read_cfg_file(server, &serv_cnt);

    FD_ZERO(&rdfs);
    FD_ZERO(&ordfs);
    for (n=0; n<serv_cnt; n++)
    {
	    if (-1 == (server[n].fd = socket(AF_INET, SOCK_STREAM, 0)))
	    	FUNW("socket");
        if (-1 == connect(server[n].fd, (struct sockaddr *)(&(server[n].addr)), sizeof(struct sockaddr)))
            FUNW("connect");

        printf("connected to connectfd:%d host:%s port:%d\n", server[n].fd, inet_ntoa(server[n].addr.sin_addr), htons(server[n].addr.sin_port));
        //printf("host %s:%d, guest %s:%d\n", inet_ntoa(serv.sin_addr), htons(serv.sin_port), inet_ntoa(guest.sin_addr), htons(guest.sin_port));

        FD_SET(server[n].fd, &ordfs);
        //connfd is the best file descriptor which in the rdfs(read file set)
        maxfd = maxfd > server[n].fd ? maxfd : server[n].fd;
    }
    FD_SET(STDIN_FILENO, &ordfs);

	while(1)
	{
		rdfs = ordfs;
		if (-1 == (ready_to = select(maxfd + 1, &rdfs, NULL, NULL, NULL)))	//select function just monit read
		{
			FUNW("select");
		}
		else
		{
            if (FD_ISSET(STDIN_FILENO, &rdfs))	//if stdin is read-ready
			{
                char str_cmd[10] ={0};
                char str_key[256] ={0};
				if (-1 == (nbytes = read(STDIN_FILENO, buf, BUFSIZE)))
					FUNW("read1");

				if (0 == strncasecmp(buf, "quit", 4))
				{
					printf("you quit!\npress any key to quit!\n");
					getchar();
					sprintf(buf, "quit!\n");
					for (n=0; n<serv_cnt; n++)   // tell all servers you quit
					{
                        if ( -1 == (server[n].fd, buf, strlen(buf), 0))
                            FUNW("write");
					}
                    bzero(buf, BUFSIZE);
                    buf[0] = '\0';
					break;
				}
				else if (0 == strncasecmp(buf, "put", 3))
				{
                        //buf = "put 100000 string100000"
                        char str_value[718] ={0};
                        sscanf(buf, "%[^ ] %[^ ] %s", str_cmd, str_key, str_value);
                        //send to server[hash]
                        size_t hash = hashInt(atoi(str_key)) % serv_cnt;
                        //if (-1 == (nbytes = send(server[hash].fd, buf, strlen(buf), 0)))
                        if (-1 == (nbytes = write(server[hash].fd, buf, strlen(buf))))
                            FUNW("write");
                        bzero(buf, BUFSIZE);
                        buf[0] = '\0';
				}
				else if (0 == strncasecmp(buf, "get", 3))
				{
                    sscanf(buf, "%[^ ] %[^ ]", str_cmd, str_key);
                    size_t hash = hashInt(atoi(str_key)) % serv_cnt;
                    //if (-1 == (nbytes = send(server[hash].fd, buf, strlen(buf), 0)))
                    if (-1 == (nbytes = write(server[hash].fd, buf, strlen(buf))))
                        FUNW("write");
                    bzero(buf, BUFSIZE);
                    buf[0] = '\0';
				}
				else if(0 == strncasecmp(buf, "del", 3))
				{
                    sscanf(buf, "%[^ ] %[^ ]", str_cmd, str_key);
                    size_t hash = hashInt(atoi(str_key)) % serv_cnt;
                    //if (-1 == (nbytes = send(server[hash].fd, buf, strlen(buf), 0)))
                    if (-1 == (nbytes = write(server[hash].fd, buf, strlen(buf))))
                        FUNW("write");
                    bzero(buf, BUFSIZE);
                    buf[0] = '\0';
				}
                if(--ready_to <= 0) continue;  // mo more readable descriptors
			}

			for (n=0; n<serv_cnt; n++)   // check all servers for data
			{
                if (FD_ISSET(server[n].fd, &rdfs))	//if connect fd is read-ready
                {
                    if (-1 == (nbytes = read(server[n].fd, buf, BUFSIZE)))
                        FUNW("read");
                    if (0 == strncasecmp(buf, "quit", 4) || 0 == nbytes)
                    {
                        printf("he quit!\npress any key to quit!\n");
                        getchar();
                        break;
                    }
                    //fprintf(stdout, "%s.%u:%s\n", inet_ntoa(server[n].addr.sin_addr), htons(server[n].addr.sin_port), buf);
                    fprintf(stdout, "%s\n", translate_result(buf));
                    bzero(buf, BUFSIZE);
                    buf[0] = '\0';
                    if(--ready_to <= 0) break;  // mo more readable descriptors
                }   // end if fd_isset
			}   // end for read server response

		}   // end else select
	}   // end while
    for (n=0; n<serv_cnt; n++)
        close(server[n].fd);
	return ((void *)NULL);
}
#endif // HASHTEST

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
        //printf(">>%s\n", str_result);
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
            if (-1 == write(info->socket, str_result, strlen(str_result)))
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

}

void *server_talk(void *arg)
{
	char **argu = (char **)arg;

	int listenfd, connectfd;    //socket information
	int maxfd;
	int port;
	int i, maxi, sockfd;
	int ready_from;
	//ssize_t n;
	int nbytes;
	struct sockaddr_in server;  // server's address information
	htpeer client[FD_SETSIZE];
	char buf[BUFSIZE];
	socklen_t addrlen;
	fd_set rdfs, allfs;

    /* create TCP socket */
	if (-1 == (listenfd = socket(AF_INET, SOCK_STREAM, 0)))
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
		if (0 > (ready_from = select(maxfd + 1, &rdfs, NULL, NULL, NULL)))	//same as peer_talk function's select
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
                if (--ready_from <= 0)  continue;   // no more readable descriptors
            }
			else if (FD_ISSET(STDIN_FILENO, &rdfs))
			{
				if (0 > (nbytes = read(STDIN_FILENO, buf, sizeof(buf))))
					FUNW("read3");

				if (0 == strncasecmp(buf, "quit", 4))
				{
					printf("you quit!\npress any key to quit!\n");
					getchar();
					sprintf(buf, "quit!\n");
					if (0 > write(connectfd, buf, strlen(buf)))
                        FUNW("write");
					break;
				}
				else if (0 == strncasecmp(buf, "conn", 4))
				{
                    //printf("IP:Port:%s,%s,%s\n", s_command, s_IP, s_port);
                    //getchar();
                    pthread_t tidc;
                    pthread_create(&tidc, NULL, peer_talk, (void *)argu);
                    pthread_join(tidc, NULL);
				}
				//buf[--nbytes] = 0;
				//printf("I:%s\n", buf);
				//if (0 > (nbytes = write(STDOUT_FILENO, buf, strlen(buf))))
                //    FUNW("write");
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
                    if (0 > nbytes)
                    {
                        FUNW("read4");
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
                        if (--ready_from <= 0)   break; /* no more readable descriptors */
                    }
                }
            }
		}
	}
	close(listenfd);
	return ((void *)NULL);
}
