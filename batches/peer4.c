//Program
//    a Distribitued Hash Table with select
//History
//  Oct-03-2015 Start
//  Oct-11-2015 End
//
/*
usage:    server:     $./test server_port
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
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

#define SERVER_FILE     "servers.cfg"
#define PEER_FILE        "peers.cfg"
#define BUFSIZE         1024    // buffer size
#define BACKLOG         20      // Number of allowed connections
#define NUMTHREADS      6       // Number of threads
#define HASHCOUNT       100000
#define TESTCOUNT        100
#define RWRWRW          (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)

#define pr_console()    printf("> ")
#define pr_msg(messsge) printf("> %s\n", message)

typedef struct htpeer {
    int fd;
    struct sockaddr_in addr; // client's address information
} htpeer;

csHashTable * table = NULL;
// hash a million strings into various sizes of table

struct timeval tval_writehash, tval_readhash, tval_deletehash, tval_done1, tval_done2;
struct timeval max_write, max_read, max_delete;
struct timeval tval_10K1, tval_10K2, tval_10K3;
tval_10K1 = max_write = (struct timeval){0};
tval_10K2 = max_read = (struct timeval){0};
tval_10K3 = max_delete = (struct timeval){0};

long int totalsize1 = 0;
long int totalsize2 = 0;
long int totalsize3 = 0;
int totalnumber1 = 0;
int totalnumber2 = 0;
int totalnumber3 = 0;
// add mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct threadinfo {
    csHashTable *table;
    int start;
} threadinfo;

typedef struct proclinfo {
    int socket;
    char* buffer;
} proclinfo;

// http://stackoverflow.com/a/12996028
// hash function for int keys
static inline long int hashInt(long int x) {
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
/*
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
*/
/* Read configuration file */
int read_cfg_file(htpeer ht_peers[], char * filename, int *servers_no)
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

    if ((fp = fopen(filename, "r")) == NULL)
    {
        printf("Error opening config file %s\n", filename);
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
            printf("Too many instances %d\n", lines);
            break;
        }
        sscanf(line, "%[^:]: %[^ ]", s_ip, s_port);
        //The server has the same IP, not support in requirement

        if (0 != strstr(s_allip, s_ip))
          continue;

        // Do not forget initialize sin_family, will result in read error
        ht_peers[lines].addr.sin_family = AF_INET;
        inet_aton(s_ip, &(ht_peers[lines].addr.sin_addr));
        ht_peers[lines].addr.sin_port = htons(atoi(s_port));
        lines++;
    }
    *servers_no = lines;
    if (0 > lines)
        printf("No instances connected.\n");
    fclose(fp);
    if (line)
        free(line);
    if(ifalladdrs)
        free(ifalladdrs);
    return 0;
}

void peer_talk(int port)
{
    int maxfd, sockfd;
    int i, maxi;
    int n;  //hash table servers cursor
    int nbytes;
    int idir;
    int listenfd, connectfd;    //socket information
    int serv_cnt;  // index servers counts
    int peer_cnt;   // peers counts
    int ready_to = 0;
    char buf[BUFSIZE];
    htpeer servers[BACKLOG];
    htpeer peers[BACKLOG];
    htpeer providers[BACKLOG];;
    fd_set rdfs, ordfs;
    socklen_t addrlen;
    struct sockaddr_in localpeer;  // current peer address information
    struct timeval timeout = {5,0};
    memset(servers, 0, BACKLOG*sizeof(htpeer));
    read_cfg_file(servers, SERVER_FILE, &serv_cnt);

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
        else {
            setsockopt(servers[n].fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(struct timeval));
            setsockopt(servers[n].fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(struct timeval));
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
        peers[n].fd = -1;providers[n].fd = -1;
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
                if (i >= maxi)   maxi = i+1;
                if (--ready_to <= 0)  continue;   // no more readable descriptors
            }
            else if (FD_ISSET(STDIN_FILENO, &rdfs))  //if stdin is read-ready
            {
                char str_cmd[10] ={0};
                char str_key[256] ={0};
                if (-1 == (nbytes = read(STDIN_FILENO, buf, BUFSIZE)))
                    FUNW("read1");

                if (0 == strncasecmp(buf, "quit", 4) )
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
                        for (n=0; n<BACKLOG; n++)   // tell all servers you quit
                        {
                            if (providers[n].fd > 0 && -1 ==write(providers[n].fd, buf, strlen(buf)))
                                FUNW("write");
                            printf("disconnected from connectfd:%d host:%s:%d\n", providers[n].fd, inet_ntoa(providers[n].addr.sin_addr), htons(providers[n].addr.sin_port));
                            FD_CLR(providers[n].fd, &ordfs);
                            providers[n].fd = -1;
                        }
                    }
                    bzero(buf, BUFSIZE);
                    buf[0] = '\0';
                }
                else if (0 == strncasecmp(buf, "prftst", 6))
                {
                    //buf = regist  1000000001 valuevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevalue
                    char str_value[718] ={0};
                    sscanf(buf, "%[^ ]", str_cmd);
                    int i;
                    timerclear(&tval_10K);
                    for (i = 1000000000; i < TESTCOUNT + 1000000000; i++)
                    {
                        bzero(buf, BUFSIZE);
                        buf[0] = '\0';
                        //send to server[hash]
                        strcpy(buf, "regist ");
                        sprintf(str_key, "%s", i);
                        strcat(buf, str_key);
                        strcat(buf, " ");
                        strcat(buf, "valuevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevalue");
                        printf("buf:%s\n", buf);
                        int hash = abs(hashString(str_folder) % serv_cnt);
                        if (0 > servers[hash].fd)
                        {
                            int newhash;
                            for(n = 1; n < serv_cnt; n++)
                            {
                                newhash = (hash+n) % serv_cnt;
                                if (servers[newhash].fd > 0) {
                                    gettimeofday(&tval_done1, NULL);
                                    if (-1 == (nbytes = write(servers[newhash].fd, buf, strlen(buf)))) {
                                        FUNW("regist 1");
                                    }
                                    bzero(buf, BUFSIZE);
                                    if (-1 == (nbytes = read(servers[newhash].fd, buf, BUFSIZE)))
                                        FUNW("read regist");
                                    gettimeofday(&tval_done2, NULL);
                                    bzero(buf, BUFSIZE);
                                    buf[0] = '\0';
                                    break;
                                }
                            }
                        }
                        else
                        {
                            gettimeofday(&tval_done1, NULL);
                            if (-1 ==  write(servers[hash].fd, buf, strlen(buf)))
                                FUNW("regist 2");
                            bzero(buf, BUFSIZE);
                            if (-1 == (nbytes = read(servers[hash].fd, buf, BUFSIZE)))
                                FUNW("read regist");
                            gettimeofday(&tval_done2, NULL);
                            bzero(buf, BUFSIZE);
                            buf[0] = '\0';
                        }
                        timersub(&tval_done2, &tval_done1, &tval_writehash);
                        timeradd(&tval_writehash, &tval_10K1, &tval_10K1);
                        max_write = (timercmp(&max_write, &tval_writehash, <) ? tval_writehash : max_write);
                        
                        //send to server[hash]
                        strcpy(buf, "search ");
                        strcat(buf, str_key);
                        strcat(buf, " ");
                        printf("buf:%s\n", buf);
                        usleep(1000);
                        if (0 > servers[hash].fd)
                        {
                            int newhash;
                            for(n = 1; n < serv_cnt; n++)
                            {
                                newhash = (hash+n) % serv_cnt;
                                if (servers[newhash].fd != -1) {
                                    gettimeofday(&tval_done1, NULL);
                                    if (-1 == (nbytes = write(servers[newhash].fd, buf, strlen(buf)))) {
                                        FUNW("search 1");
                                    bzero(buf, BUFSIZE);
                                    buf[0] = '\0';
                                    if (-1 == (nbytes = read(servers[newhash].fd, buf, BUFSIZE)))
                                        FUNW("read search");
                                    }
                                    break;
                                }
                            }
                        }
                        else
                        {
                            //printf("servers[%d].fd:%d\n", hash, servers[hash].fd);
                            gettimeofday(&tval_done1, NULL);
                            if (-1 ==  write(servers[hash].fd, buf, strlen(buf)))
                                FUNW("search 2");
                            bzero(buf, BUFSIZE);
                            buf[0] = '\0';
                            if (-1 == (nbytes = read(servers[hash].fd, buf, BUFSIZE)))
                                FUNW("read search");
                            gettimeofday(&tval_done2, NULL);
                            fprintf(stdout, "%s\n", buf);
                        }
                        bzero(buf, BUFSIZE);
                        buf[0] = '\0';
                        timersub(&tval_done2, &tval_done1, &tval_readhash);
                        timeradd(&tval_readhash, &tval_10K2, &tval_10K2);
                        max_read = (timercmp(&max_read, &tval_readhash, <) ? tval_readhash : max_read);
                        
                        //send to server[hash]
                        strcpy(buf, "delete ");
                        strcat(buf, str_key);
                        strcat(buf, " ");
                        printf("buf:%s\n", buf);
                        usleep(1000);
                        if (0 > servers[hash].fd)
                        {
                            int newhash;
                            for(n = 1; n < serv_cnt; n++)
                            {
                                newhash = (hash+n) % serv_cnt;
                                if (servers[newhash].fd != -1) {
                                    gettimeofday(&tval_done1, NULL);
                                    if (-1 == (nbytes = write(servers[newhash].fd, buf, strlen(buf)))) {
                                        FUNW("delete 1");
                                    bzero(buf, BUFSIZE);
                                    buf[0] = '\0';
                                    if (-1 == (nbytes = read(servers[newhash].fd, buf, BUFSIZE)))
                                        FUNW("read delete");
                                    }
                                    break;
                                }
                            }
                        }
                        else
                        {
                            //printf("servers[%d].fd:%d\n", hash, servers[hash].fd);
                            gettimeofday(&tval_done1, NULL);
                            if (-1 ==  write(servers[hash].fd, buf, strlen(buf)))
                                FUNW("delete 2");
                            bzero(buf, BUFSIZE);
                            buf[0] = '\0';
                            if (-1 == (nbytes = read(servers[hash].fd, buf, BUFSIZE)))
                                FUNW("read delete");
                            gettimeofday(&tval_done2, NULL);
                            fprintf(stdout, "%s\n", buf);
                        }
                        bzero(buf, BUFSIZE);
                        buf[0] = '\0';
                        timersub(&tval_done2, &tval_done1, &tval_deletehash);
                        timeradd(&tval_deletehash, &tval_10K2, &tval_10K2);
                        max_delete = (timercmp(&max_delete, &tval_deletehash, <) ? tval_deletehash : max_delete);
                    }
                    printf("MAX write in 16 index servers: %ld.%06ld sec\n", (long int)max_write.tv_sec, (long int)max_write.tv_usec);
                    printf("MAX read in 16 index servers: %ld.%06ld sec\n", (long int)max_read.tv_sec, (long int)max_read.tv_usec);
                    printf("MAX delete in 16 index servers: %ld.%06ld sec\n", (long int)max_delete.tv_sec, (long int)max_delete.tv_usec);
                    printf("SET %d files in 16 index servers: %ld.%06ld sec\n", j, (long int)tval_10K1.tv_sec, (long int)tval_10K1.tv_usec);
                    printf("GET %d files in 16 index servers: %ld.%06ld sec\n", j, (long int)tval_10K2.tv_sec, (long int)tval_10K2.tv_usec);
                    printf("DEL %d files in 16 index servers: %ld.%06ld sec\n", j, (long int)tval_10K3.tv_sec, (long int)tval_10K3.tv_usec);
                    bzero(buf, BUFSIZE);
                    buf[0] = '\0';
                }
                if(--ready_to <= 0) continue;  // mo more readable descriptors
            }


            for (i = 0; i < maxi; i++)     // check all clients for data
            {
                if ((sockfd = peers[i].fd) < 0)    continue;
                if (FD_ISSET(sockfd, &rdfs))
                {
                    bzero(buf, BUFSIZE);
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
    if (2 == argc)    //argc == 2, instructing that this is server
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
