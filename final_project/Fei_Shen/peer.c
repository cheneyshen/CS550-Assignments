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
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
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
#define NUMTHREADS      6       // Number of threads
#define HASHCOUNT       100000
#define RWRWRW          (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)

#define pr_console()    printf("> ")
#define pr_msg(messsge) printf("> %s\n", message)

typedef struct htpeer {
    int fd;
    struct sockaddr_in addr; // client's address information
} htpeer;

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

typedef struct proclinfo {
    int socket;
    char* buffer;
} proclinfo;

// http://stackoverflow.com/a/12996028
// hash function for int keys
static inline long int hashInt(long int x)
{
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x);
	return x;
}

// http://www.cse.yorku.ca/~oz/hash.html
// hash function for string keys djb2
static inline long int hashString(char * str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

// helper for converting hash result into readable string
// without static for reference
static inline char* translate_result(char * result)
{
    if (isdigit(result[0]) && strlen(result))
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

static inline void print_result(char *result)
{
    char *str1, *str2 , *token, *subtoken;
    char *saveptr1, *saveptr2;
    int j, n=0;

    for (j = 1, str1 = result; ; j++, str1 = NULL) {
        token = strtok_r(str1, "^", &saveptr1);
        if (token == NULL)
        break;
        n=0;
        for (str2 = token; ; str2 = NULL) {
            n++;
            subtoken = strtok_r(str2, "$", &saveptr2);
            if (subtoken == NULL)
            break;
            if (1==n) printf("%s\t", subtoken);
            else printf("%s\n", subtoken);
        }
    }

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
        if(lines > BACKLOG)
        {
            printf("Too many hash table servers %d\n", lines);
            break;
        }
        sscanf(line, "%[^:]: %[^ ]", s_ip, s_port);
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

void * get_file(void *arg)
{
    proclinfo *info = arg;
    int filesize;
    int fd;
    int wn = 0;
    char filebuf[BUFSIZE];  // used to read file
    char fileinfo[20] = {0};
    char filename[256] = {0};
    char filepath[256] = {0};
    char *ptr, r = '/';
    FILE *fp;
    struct timeval timeout = {5,0};
    //set recv timeout
    setsockopt(info->socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(struct timeval));

    if(-1 == write(info->socket, info->buffer, BUFSIZE))
        FUNW("write");
    sscanf(info->buffer, "%[^ ] %[^ ]", fileinfo, filename);
    if (-1 == recv(info->socket, fileinfo, sizeof(fileinfo), 0))
        FUNW("recv");
    printf("filesize:%s\n", fileinfo);
    if((filesize = atoi(fileinfo)) == 0) {
        printf("get file size error!");
    }
    //Save to ./Download/ folder
    sprintf(filepath, "./Download");
    if (-1 == access(filepath, X_OK))
    {
        int status = mkdir(filepath, S_IRWXU | S_IRWXG | S_IRWXO); /*777*/
        (!status) ? (printf("Directory created %s\n", filepath)) : (printf("Unable to create directory %s\n", filepath));
    }
    ptr = strrchr(filename, r);
    strcat(filepath, ptr);

    if( ( fp = fopen(filepath,"wb") ) == NULL )
    {
        perror("File");
        free(info);
        pthread_exit(NULL);
    }
    printf("Downloading %s...\n", filename);
    while( 1 )
    {
        wn = recv(info->socket, filebuf, BUFSIZE, 0);
        fwrite(filebuf, 1, wn, fp);
        filesize -= wn;
        if (filesize  == 0 )    // failed connection processing
            break;
        else if( 0 > wn || (wn == 0 && filesize > 0) ) {
            printf("Lost connection to socket fd: %d provider\n", info->socket);
            break;
        }
    }
    if(-1 == fclose(fp))
        FUNW("fclose");
    printf("Done!\n");
    free(info);
    pthread_exit(NULL);
}

void * put_file(void *arg)
{
    proclinfo *info = arg;
    char filebuf[BUFSIZE];      // used to read file
    char filename[256] = {0};   // path + filename
    char filesize[20] = {0};
    char str_opr[10] = {0};
    int fd;
    int len, sendlen;
    struct stat buff;       // information about a file
    FILE *fq;
    sscanf(info->buffer, "%[^ ] %[^ ]", str_opr, filename);
    struct timeval timeout = {5,0};
    //set send timeout
    setsockopt(info->socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(struct timeval));

    if( (fq = fopen(filename,"rb")) == NULL )
    {
            perror("fopen");
            free(info);
            pthread_exit(NULL);
    }
    if (lstat(filename, &buff) < 0)
        FUNW("lstat");
    snprintf(filesize, sizeof(filesize), " %ld", buff.st_size);
    send(info->socket, filesize, sizeof(filesize), 0);  // send file size

    bzero(filebuf, sizeof(filebuf));
    printf("Uploading %s...\n", filename);
    while( !feof(fq) )
    {
        len = fread(filebuf,1,BUFSIZE,fq);
        sendlen = send(info->socket, filebuf, len, 0);
        //printf("sendlen=%d\n", sendlen);
        if( len !=  sendlen)
        {
            perror("write"); break;
        }
    }
    if(-1 == fclose(fq))
        FUNW("fclose");
    printf("Finished!\n");
    free(info);
    pthread_exit(NULL);
}

void peer_talk(int port)
{
    int maxfd, sockfd;
    int i, maxi;
    int n;  //hash table servers cursor
    int nbytes;
    int listenfd, connectfd;    //socket information
    int serv_cnt;  // index servers counts
    int ready_to = 0;
    char buf[BUFSIZE];
    htpeer servers[BACKLOG];
    htpeer peers[BACKLOG];
    htpeer provider;
    fd_set rdfs, ordfs;
    socklen_t addrlen;
    struct sockaddr_in localpeer;  // current peer address information
    struct timeval timeout = {5,0};
    memset(servers, 0, BACKLOG*sizeof(htpeer));
    read_cfg_file(servers, &serv_cnt);

    FD_ZERO(&rdfs);
    FD_ZERO(&ordfs);
    // Set timeout threshold
    for (n = 0; n < serv_cnt; n++)
    {
        if (-1 == (servers[n].fd = socket(AF_INET, SOCK_STREAM, 0)))
            FUNW("socket");
        if (-1 == connect(servers[n].fd, (struct sockaddr *)(&(servers[n].addr)), sizeof(struct sockaddr)))
        {
            printf("Failed to connect to %s:%d\n", inet_ntoa(servers[n].addr.sin_addr), htons(servers[n].addr.sin_port));
            servers[n].fd = -1;
        }
        else{
            setsockopt(servers[n].fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(struct timeval));
            printf("Connected to connectfd:%d host:%s:%d\n", servers[n].fd, inet_ntoa(servers[n].addr.sin_addr), htons(servers[n].addr.sin_port));
            FD_SET(servers[n].fd, &ordfs);
            //connfd is the best file descriptor which in the rdfs(read file set)
            maxfd = maxfd > servers[n].fd ? maxfd : servers[n].fd;
            ready_to++;
        }
    }
    if (!ready_to)  // no servers connected
        return;

    /* create TCP socket */
    if (-1 == (listenfd = socket(AF_INET, SOCK_STREAM, 0)))
        FUNW("socket");
    int opt = SO_REUSEADDR;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero(&localpeer, sizeof(localpeer));
    localpeer.sin_family = AF_INET;
    localpeer.sin_port = htons(port);
    localpeer.sin_addr.s_addr = htonl(INADDR_ANY);
    if (-1 == bind(listenfd, (struct sockaddr *)(&localpeer), sizeof(localpeer)))
        FUNW("bind");
    if (-1 == listen(listenfd, BACKLOG))
        FUNW("listen");
    addrlen = sizeof(struct sockaddr_in);

    maxfd = maxfd > listenfd ? maxfd : listenfd;
    maxi = -1;
    FD_SET(listenfd, &ordfs);
    FD_SET(STDIN_FILENO, &ordfs);

    for (n = 0; n < BACKLOG; n++)
    {
        peers[n].fd = -1;
    }

    while(1)
    {
        struct sockaddr_in addr;
        rdfs = ordfs;
        if (0 >  (ready_to = select(maxfd + 1, &rdfs, NULL, NULL, NULL)))  //select function just monitor read
        {
            FUNW("Select");
        }
        else
        {
            if (FD_ISSET(listenfd, &rdfs))  //if listenfd is read_ready, we can accept new connection
            {
                // accept connection
                if (-1 == (connectfd = accept(listenfd, (struct sockaddr *)(&addr), &addrlen)))
                {
                    FUNW("accept");
                }
                //put new fd to client
                for (i = 0; i < BACKLOG; i++)
                {
                    if (peers[i].fd < 0)
                    {
                        peers[i].fd = connectfd;   // save descriptor
                        //peers[i].name = (char *)malloc(BUFSIZE);  //update for Segment Fault(core dumped)
                        memcpy((struct sockaddr *)&(peers[i].addr), &addr, sizeof(struct sockaddr));
                        printf("connected from %d host:%s:%d\n", peers[i].fd, inet_ntoa(peers[i].addr.sin_addr), htons(peers[i].addr.sin_port));
                        break;
                    }
                }
                if (i == BACKLOG)
                    FUNW("too many peers");
                FD_SET(peers[i].fd, &ordfs);  // add new connect descriptor to set
                maxfd = maxfd > peers[i].fd ? maxfd : peers[i].fd;  //if connectfd is greater than maxfd, then change them
                if (i >= maxi)   maxi = i+1;    // must be no less than, otherwise can't receive socket from other peers.
                if (--ready_to <= 0)  continue;   // no more readable descriptors
            }
            else if (FD_ISSET(STDIN_FILENO, &rdfs))  //if stdin is read-ready
            {
                char str_cmd[10] ={0};
                char str_key[256] ={0};
                if (-1 == (nbytes = read(STDIN_FILENO, buf, BUFSIZE)))
                    FUNW("read1");

                if (0 == strncasecmp(buf, "quit", 4) || 0 == strncasecmp(buf, "q", 1))
                {
                    sscanf(buf, "%[^ ] %[^ ]", str_cmd, str_key);
                    if (0 == strncasecmp(str_key, "server", 6))
                    {
                        printf("Press any key to close connections to index servers!\n");
                        getchar();
                        sprintf(buf, "quit!\n");
                        for (n=0; n<serv_cnt; n++)   // tell all servers you quit
                        {
                            if (servers[n].fd > 0 && -1 ==write(servers[n].fd, buf, strlen(buf)))
                                FUNW("write");
                        }
                        break;
                    }
                    else if (0 == strncasecmp(str_key, "peer", 4))
                    {
                        printf("Press any key to close connection to peer!\n");
                        getchar();
                        sprintf(buf, "quit!\n");
                        if (provider.fd > 0 && -1 ==write(provider.fd, buf, strlen(buf)))
                            FUNW("write");
                        printf("disconnected from connectfd:%d host:%s:%d\n", provider.fd, inet_ntoa(provider.addr.sin_addr), htons(provider.addr.sin_port));
                        FD_CLR(provider.fd, &ordfs);
                        provider.fd = -1;
                    }
                    bzero(buf, BUFSIZE);
                    buf[0] = '\0';
                }
                else if (0 == strncasecmp(buf, "obtain", 6) || 0 == strncasecmp(buf, "o", 1))
                {
                    //buf = "obtain  /home/sf/aaa.txt 127.0.0.1:8888"
                    memset(&provider, 0, sizeof(htpeer));
                    char str_value[718] ={0};
                    char s_ip[16] = {0};
                    char s_port[5] = {0};
                    sscanf(buf, "%[^ ] %[^ ] %s", str_cmd, str_key, str_value);
                    // Do not forget initialize sin_family, will result in read error
                    sscanf(str_value, "%[^:]: %[^ ]", s_ip, s_port);
                    provider.addr.sin_family = AF_INET;
                    inet_aton(s_ip, &(provider.addr.sin_addr));
                    provider.addr.sin_port = htons(atoi(s_port));
                    if (-1 == (provider.fd = socket(AF_INET, SOCK_STREAM, 0)))
                        FUNW("socket");
                    if (-1 == connect(provider.fd, (struct sockaddr *)(&(provider.addr)), sizeof(struct sockaddr)))
                        FUNW("connect");
                    printf("Connected to connectfd:%d host:%s:%d\n", provider.fd, inet_ntoa(provider.addr.sin_addr), htons(provider.addr.sin_port));
                    FD_SET(provider.fd, &ordfs);
                    //connfd is the best file descriptor which in the rdfs(read file set)
                    maxfd = maxfd > provider.fd ? maxfd : provider.fd;
                    buf[nbytes - 1] = '\0'; // read nbytes bytes
                    pthread_t *pth = malloc(sizeof(pthread_t));
                    proclinfo *info = (proclinfo*)malloc(sizeof(proclinfo));
                    buf[strlen(buf) - strlen(str_value)] = '\0';
                    info->buffer = buf;
                    //printf("buf:%s\n", buf);
                    info->socket = provider.fd;
                    pthread_create(pth, NULL, get_file, info);
                    pthread_join(*pth, NULL);
                }
                else if (0 == strncasecmp(buf, "regist", 6) || 0 == strncasecmp(buf, "r", 1))
                {
                    //buf = "regist  /home/sf/aaa.txt 127.0.0.1:8888"
                    char str_value[718] ={0};
                    sscanf(buf, "%[^ ] %[^ ] %s", str_cmd, str_key, str_value);
                    //send to server[hash]
                    int hash = abs(hashString(str_key) % serv_cnt);
                    if (-1 == servers[hash].fd)
                    {
                        int newhash;
                        for(n = 1; n < serv_cnt; n++)
                        {
                            newhash = (hash+n) % serv_cnt;
                            if (servers[newhash].fd != -1) {
                                if (-1 == (nbytes = write(servers[newhash].fd, buf, strlen(buf)))) {
                                    FUNW("write");
                                }
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (-1 ==  write(servers[hash].fd, buf, strlen(buf)))
                            FUNW("write");
                    }
                    bzero(buf, BUFSIZE);
                    buf[0] = '\0';
                }
                else if (0 == strncasecmp(buf, "search", 6) || 0 == strncasecmp(buf, "s", 1))
                {
                    //buf = "search  aaa.txt"
                    sscanf(buf, "%[^ ] %[^ ]", str_cmd, str_key);
                    for(n = 0; n < serv_cnt; n++)
                    {
                        if (servers[n].fd != -1) {
                            nbytes = write(servers[n].fd, buf, strlen(buf));
                            //printf("nbytes:%d\n", nbytes);
                            break;
                            }
                    }
                    bzero(buf, BUFSIZE);
                    buf[0] = '\0';
                }
                else if (0 == strncasecmp(buf, "delete", 6) || 0 == strncasecmp(buf, "d", 1))
                {
                    //buf = "delete  /home/sf/aaa.txt"
                    sscanf(buf, "%[^ ] %[^ ]", str_cmd, str_key);
                    int hash = abs(hashString(str_key) % serv_cnt);
                    if (-1 == servers[hash].fd)
                    {
                        int newhash;
                        for(n = 1; n < serv_cnt; n++)
                        {
                            newhash = (hash+n) % serv_cnt;
                            if (servers[newhash].fd != -1) {
                                if (-1 == (nbytes = write(servers[newhash].fd, buf, strlen(buf)))) {
                                    //printf("nbytes:%d\n", nbytes);
                                    FUNW("write");
                                }
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (-1 ==  write(servers[hash].fd, buf, strlen(buf)))
                            FUNW("write");
                    }
                    bzero(buf, BUFSIZE);
                    buf[0] = '\0';
                }
                if(--ready_to <= 0) continue;  // mo more readable descriptors
            }

            for (n=0; n<serv_cnt; n++)   // check all servers for data
            {
                if ((sockfd = servers[n].fd) < 0)    continue;
                if (FD_ISSET(sockfd, &rdfs))  //if connect fd is read-ready
                {
                    bzero(buf, BUFSIZE);
                    buf[0] = '\0';
                    if (-1 == (nbytes = read(sockfd, buf, BUFSIZE)))
                        FUNW("read");
                    if (0 == strncasecmp(buf, "quit", 4) || 0 == nbytes)
                    {
                        printf("(Index Server)%s:%dquit!\n", inet_ntoa(servers[n].addr.sin_addr), htons(servers[n].addr.sin_port));
                        FD_CLR(servers[n].fd, &ordfs);
                        servers[n].fd = -1;
                    }
                    else if (0 == strncasecmp(buf, "obtain", 6) || 0 == strncasecmp(buf, "o", 1))
                    {
                        // buf = "obtain /home/sf/text.txt"
                        buf[nbytes] = '\0'; // read nbytes bytes
                        pthread_t *pth = malloc(sizeof(pthread_t));
                        proclinfo *info = (proclinfo*)malloc(sizeof(proclinfo));
                        info->buffer = buf; info->socket = sockfd;
                        pthread_create(pth, NULL, put_file, info);
                        pthread_join(*pth, NULL);
                    }
                    else if (0 != strstr(buf, "^") || 0 != strstr(buf, "$")) {
                        print_result(buf);
                    }
                    else {
                        fprintf(stdout, "%s\n", translate_result(buf));
                    }
                    bzero(buf, BUFSIZE);
                    buf[0] = '\0';
                    if (--ready_to <= 0) break;  // mo more readable descriptors
                }   // end if fd_isset
            }   // end for read server response

            for (i = 0; i < maxi; i++)     // check all clients for data
            {
                if ((sockfd = peers[i].fd) < 0)    continue;
                if (FD_ISSET(sockfd, &rdfs))
                {
                    bzero(buf, BUFSIZE);
                    buf[0] = '\0';
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
                            fprintf(stdout, "Peer:%s:%d quit!\n",  inet_ntoa(peers[i].addr.sin_addr), htons(peers[i].addr.sin_port));
                            close(sockfd);
                            FD_CLR(sockfd, &ordfs);
                            peers[i].fd = -1;
                        }
                        else if (0 == strncasecmp(buf, "obtain", 6) || 0 == strncasecmp(buf, "o", 1))
                        {
                            // buf = "obtain /home/sf/text.txt"
                            buf[nbytes] = '\0'; // read nbytes bytes
                            pthread_t *pth = malloc(sizeof(pthread_t));
                            proclinfo *info = (proclinfo*)malloc(sizeof(proclinfo));
                            info->buffer = buf; info->socket = sockfd;
                            //printf("buffer:%s", buf);
                            pthread_create(pth, NULL, put_file, info);
                            pthread_join(*pth, NULL);
                        }
                        if (--ready_to <= 0)   break; /* no more readable descriptors */
                    }
                }
            }
        }   // end else select
   }   // end while

    for (n=0; n<serv_cnt; n++)
        close(servers[n].fd);
    //return ((void *)NULL);
}

int main(int argc, char **argv)
{
    #ifdef HASHTEST
        printf("In TEST mode\n");
    #endif
    if (2 == argc)	//argc == 2, instructing that this is server
    {
        int iport;
        if (-1 == (iport = atoi(argv[1])))    // get  port
            FUNW("atoi");
        peer_talk(iport);
    }
    else
    {
        printf("Uasge: %s port\n", argv[0]);
    }
    return 0;
}
