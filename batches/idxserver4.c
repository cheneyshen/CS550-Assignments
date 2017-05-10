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

#define CONFIG_FILE     "replicate.cfg"
#define BUFSIZE         1024    // buffer size
#define BACKLOG         10      // Number of allowed connections
#define BACKPORT    9999
#define RSTSIZE         8192

#define pr_console()    printf("> ")
#define pr_msg(messsge) printf("> %s\n", message)

typedef struct htpeer {
    int fd;
    struct sockaddr_in addr; // client's address information
} htpeer;

typedef struct threadinfo {
    csHashTable *table;
    unsigned int start;
    int socket;
    int bckskt;
    union
    {
        char  *strValue;
        double dblValue;
        int    intValue;
        void  *ptrValue;
    } key;
    union
    {
        char  *strValue;
        double dblValue;
        int    intValue;
        void  *ptrValue;
    } value;
} threadinfo;

typedef struct proclinfo {
    int socket;
    int bckskt;
    char* buffer;
    csHashTable *table;
} proclinfo;

//void *server_talk(void *);    //server part in this program

// hash a million strings into various sizes of table
struct timeval tval_before, tval_done1, tval_done2, tval_done3, tval_done4, tval_done5;
struct timeval tval_writehash, tval_readhash, tval_deletehash;
int registtime;
int searchtime;

// http://stackoverflow.com/a/12996028
// hash function for int keys
static inline long int hashInt(long int x)
{
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x);
    return x;
}

// helper for copying string keys and values
static inline char * copystring(char * value)
{
    char * copy = (char *)malloc(strlen(value)+1);
    if (!copy) {
        printf("Unable to allocate string value %s\n",value);
        abort();
    }
    strcpy(copy,value);
    return copy;
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

char rstmsg[RSTSIZE] = {'\0'};

// add mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void * thread_search(void *arg)
{
    threadinfo *info = arg;
    unsigned int i = info->start;
    char * buffer = (char *)malloc(RSTSIZE);
    char *strkey = info->key.strValue;
    if (!buffer) {
        printf("Unable to allocate buffer\n");
        abort();
    }
    csHashTable *table = info->table;
    free(info);
    info->value.intValue = get_set_by_str(table, i, strkey, &buffer);
    //printf("intValue:%d\n",  info->value.intValue);
    if (HASHOK == info->value.intValue)
    {
        // lock
        if(pthread_mutex_lock(&mutex) != 0)
        {
          perror("pthread_mutex_lock");
          exit(EXIT_FAILURE);
        }
        strcat(rstmsg, buffer);
        // unlock
        if(pthread_mutex_unlock(&mutex) != 0)
        {
            perror("pthread_mutex_unlock");
            exit(EXIT_FAILURE);
        }
    }
    else
        free(buffer);
    return ((threadinfo *)info);
}

void * process_cli(void *arg)
{
    proclinfo *info = arg;
    char str_opr[10] = {0};
    char str_key[256] = {0};
    //char *str_value;
    int result = -1;
    char str_result[BUFSIZE] = {0};
    if (0 == strncasecmp(info->buffer, "regist", 6))
    {
        char str_value[718] = {0};
        sscanf(info->buffer, "%[^ ] %[^ ] %s", str_opr, str_key, str_value);
        result = add_str_by_str(info->table, str_key, str_value);
        sprintf(str_result, "%d", result);
        //printf("result=%s\n", str_result);
       if (-1 == write(info->socket, str_result, strlen(str_result)))
            FUNW("regist back");

    }
    else if (0 == strncasecmp(info->buffer, "search", 6) )
    {
        sscanf(info->buffer, "%[^ ] %s", str_opr, str_key);     // %[^ ] %[^ ] will cover enter
        pthread_t * threads[NUMTHREADS];
        unsigned int t;int status[NUMTHREADS];
        bzero(rstmsg, RSTSIZE);
        rstmsg[0] = '\0';
        for(t = 0; t < NUMTHREADS; t++) {
            pthread_t * pth = malloc(sizeof(pthread_t));
            threads[t] = pth;
            threadinfo *tinfo = (threadinfo*)malloc(sizeof(threadinfo));
            tinfo->table = info->table;
            tinfo->start = t;
            tinfo->socket = info->socket;
            tinfo->key.strValue = copystring(str_key);
            pthread_create(pth, NULL, thread_search, tinfo);    //tinfo without & the address-of operator
            pthread_join(*threads[t], NULL);
        }
        if(0 == strlen(rstmsg)) {
            sprintf(rstmsg, "%d", HASHNOTFOUND);
        }
        //printf("rstmsg:%s\n", rstmsg);
        if (-1 == write(info->socket, rstmsg, strlen(rstmsg)))
            FUNW("search back");

    }
    else if (0 == strncasecmp(info->buffer, "delete", 6))
    {
        sscanf(info->buffer, "%[^ ] %s", str_opr, str_key);
        result = del_by_str(info->table, str_key);
        if (HASHNOTFOUND == result) {
            sprintf(str_result, "%d", result);
            if (-1 == write(info->socket, str_result, strlen(str_result)))
                FUNW("write");
        }
        else
        {
            sprintf(str_result, "%d", result);
            if (-1 == write(info->socket, str_result, strlen(str_result)))
                FUNW("write");
        }
        bzero(str_result, BUFSIZE);
    }

    free(info);
    pthread_exit(NULL);
}

void server_talk(csHashTable *htable, int backfd, int port)
{
    int listenfd, connectfd;    //socket information
    int maxfd;
    int i, maxi, sockfd;
    int ready_from;
    //ssize_t n;
    int nbytes;
    struct sockaddr_in server;  // server's address information
    htpeer client[BACKLOG];
    char buf[BUFSIZE];
    socklen_t addrlen;
    fd_set rdfs, allfs;

    /* create TCP socket */
    if (-1 == (listenfd = socket(AF_INET, SOCK_STREAM, 0)))
    FUNW("socket");

    int opt = SO_REUSEADDR;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero(&server, sizeof(server));
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
    for (i = 0; i < BACKLOG; i++)
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
        if (0 > (ready_from = select(maxfd + 1, &rdfs, NULL, NULL, NULL)))    //same as peer_talk function's select
        {
            FUNW("select");
        }
        else
        {
            if (FD_ISSET(listenfd, &rdfs))    //if listenfd is read_ready, we can accept new connection
            {
                /* accept connection */
                if (-1 == (connectfd = accept(listenfd, (struct sockaddr *)(&addr), &addrlen)))
                {
                    FUNW("accept");
                }
                /* put new fd to client */
                for (i = 0; i < BACKLOG; i++)
                    if (client[i].fd < 0)
                    {
                        client[i].fd = connectfd;   // save descriptor
                        //client[i].name = (char *)malloc(BUFSIZE);  //update for Segment Fault(core dumped)
                        memcpy((struct sockaddr *)&(client[i].addr), &addr, sizeof(struct sockaddr));
                        printf("Connected from host:%s:%d\n", inet_ntoa(client[i].addr.sin_addr), htons(client[i].addr.sin_port));
                        break;
                    }
                if (i == BACKLOG)
                    printf("too many clients.\n");
                FD_SET(connectfd, &allfs);  // add new connect descriptor to set
                maxfd = maxfd > connectfd ? maxfd : connectfd;    //if connectfd is greater than maxfd, then change them
                if (i >= maxi)   maxi = i+1;
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
                    for (i = 0; i < maxi; i++)     // check all clients for data
                    {
                        if (client[i].fd != -1 && FD_ISSET(client[i].fd, &rdfs) && -1 == write(sockfd, buf, strlen(buf)))
                            FUNW("Write quit");
                    }
                    if (backfd > 0 && -1 == write(backfd, buf, strlen(buf)))
                        FUNW("Write backup quit");
                    break;
                }   // end strncasecmp

            } // end FD_ISSET
            for (i = 0; i < maxi; i++)     // check all clients for data
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
                            fprintf(stdout, "Client:%s:%d quit!\n",  inet_ntoa(client[i].addr.sin_addr), htons(client[i].addr.sin_port));
                            close(sockfd);
                            FD_CLR(sockfd, &allfs);
                            client[i].fd = -1;
                        }
                        else
                        {
                            buf[nbytes] = '\0'; // read nbytes bytes
                            pthread_t * pth = malloc(sizeof(pthread_t));
                            proclinfo *info = (proclinfo*)malloc(sizeof(proclinfo));
                            info->table = htable; info->buffer = buf; info->socket = sockfd; info->bckskt = backfd;
                            pthread_create(pth, NULL, process_cli, info);
                            pthread_join(*pth, NULL);

                        }
                        if (--ready_from <= 0)   break; /* no more readable descriptors */
                    }
                }
            }
        }
    }
    close(listenfd);
    //return ((void *)NULL);
}

/* Read configuration file */
int read_cfg_file(htpeer *ht_peer)
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
        return -1;
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
        if(lines > 2)
        {
            printf("Too many back up servers %d\n", lines);
            break;
        }
        sscanf(line, "%[^:]: %[^ ]", s_ip, s_port);
        //The server has the same IP, not support in requirement
        #ifdef HASHTEST
                if (0 != strstr(s_allip, s_ip))
                  continue;
        #endif
        // Do not forget initialize sin_family, will result in read error
        ht_peer->addr.sin_family = AF_INET;
        inet_aton(s_ip, &(ht_peer->addr.sin_addr));
        ht_peer->addr.sin_port = htons(atoi(s_port));
    }
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
    if (2 == argc)  //argc == 2, instructing that this is server
    {
        //initialize server's hashtable
        csHashTable * table = NULL;
        table = create_hash(HASHCOUNT>>4);   // 100000/16 buckets per server
        //table = create_hash(9973);    // 9973 is a prime number
        if (!table) {
            // fail
            return 1;
        }
        int port;
        if (-1 == (port = atoi(argv[1])))    // get server port
            FUNW("atoi");
        if (BACKPORT != port)
        {
            htpeer ht_back;
            memset(&ht_back, 0, sizeof(htpeer));
            if (-1 == read_cfg_file(&ht_back))
                return 0;
            int backfd;
            struct timeval timeout = {5,0};
            if (-1 == (backfd = socket(AF_INET, SOCK_STREAM, 0)))
                FUNW("socket");
            if (-1 == connect(backfd, (struct sockaddr *)(&(ht_back.addr)), sizeof(struct sockaddr)))
            {
                printf("Failed to connect to back server %s:%d\n", inet_ntoa(ht_back.addr.sin_addr), htons(ht_back.addr.sin_port));
                backfd = -1;
            }
            else{
                setsockopt(backfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(struct timeval));
                printf("Connected to backup server:%s:%d\n", inet_ntoa(ht_back.addr.sin_addr), htons(ht_back.addr.sin_port));
            }
            server_talk(table, backfd, port);
        }
        else
            server_talk(table, -1, port);
    }
    else
    {
        printf("Uasge: %s port\n", argv[0]);
    }
    return 0;
}
