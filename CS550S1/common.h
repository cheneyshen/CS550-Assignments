/*
 *  comon.h is a used for creating a common library
 *  for server and client
 *  start_time: 03.Sep.2015
 *  start_time: 21.Sep.2015
 */
#ifndef _COMMON_H

#define _COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>    // for inet_ntoa
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>   // for mkdir
#include <fcntl.h>      // for open
#include <errno.h>      // for errno
#include <time.h>       // for ctime
#include <signal.h>     // for kill
//#include <sys/ioctl.h>
//#include <net/if.h>
//#include <math.h>
//#include <netinet/in.h>
//#include <inttypes.h>

#define SEVR_PORT       8081
#define SEVR_ID         0
#define CNTNT_LEN       8192
#define MSG_LEN         sizeof(struct msg)
#define ADDR_LEN        sizeof(struct sockaddr)
//#define USR_LEN         sizeof(struct user)
#define PEER_LEN        sizeof(struct peer)
//#define PRT_LEN         8
#define FL_LEN          sizeof(struct file)

/* declare Global variables */
int sockfd;/* used as socket local handle */
int count;
struct sockaddr_in server;
int peerNum;
int fileNum;

/* msg is used for communicating message */
struct msg
{
    /*
     * flag meaning:1,register, 2,log in,
     * 3,chat msg, 4,share file, 5,search file,
     * 6,peer addr, 7,file send, 8, file receive
     */
    int flag;
    unsigned int id_from;
    unsigned int id_to;
    char content[CNTNT_LEN];
    char append[24];
};

/* peer is used for peer information list */
struct peer
{
    unsigned int id; // peer id
    char name[10];
};

struct file
{
    unsigned int id; // file id
    unsigned int owner;  // peer id
    unsigned int filesize;  // file size
    char name[256];  //file absolute path+name
};

/* common functions begin */
int i_clean_stdin ()
{
    while ('\n' == getchar())
    {
        continue;
    }
    return(0);
}

int i_print(char *pmsg, int size)
{
    int i;

    for (i = 1; i<= size; i++)
    {
        if (*pmsg != '\n')
        {
            printf("%c", *pmsg);
            pmsg ++;
        }
        else
        {
            return(0);
        }
    }

    return(0);
}

int i_input(char *p_input)
{
    char c = '\0';
    int i;

    for (i = 0; i < CNTNT_LEN; i++)
    {
        p_input[i] = getchar();
        if (p_input[i] =='\n')
        {
            return(0);
        }
    }
    p_input[i] ='\n';
    printf("you have input long enough!\n");
    return(0);
}

int i_socket(int domain, int type, int protocol)
{
    int fd;

    if ((fd = socket(domain, type, protocol)) == -1)
    {
        perror("creat socket error:");
        exit(1);
    }

    return(fd);
}

int i_bind(int fd, const struct sockaddr *addr, int namelen)
{
    if (-1 == bind(fd, addr, namelen))
    {
        perror("i_bind error:");
        exit(1);
    }

    return (0);
}

int i_recvfrom(int fd, void *buf, size_t len, int flags,
        struct sockaddr *addr, int *size)
{
    int pos = recvfrom(fd, buf, len, flags, addr, (socklen_t *)size);
    if (-1 ==pos)
    {
        perror("i_recvfrom error:");
        exit(1);
    }

    return(pos);
}

int i_sendto(int fd, void *buf, size_t len, int flags,
        struct sockaddr *addr, int size)
{
    int pos = sendto(fd, buf, len, flags, addr, size);
    if (-1 == pos)
    {
        perror("i_sendto error");
        exit(1);
    }

    return (pos);
}

off_t i_lseek(int fd, off_t size, int position)
{
    off_t curpos;
    curpos = lseek(fd, size, position);
    if (-1 == curpos)
    {
        perror("seek error");
        return(-1);
    }
    return(curpos);
}

int i_fseek(FILE *stream, long offset, int whence)
{
    int value = fseek(stream, offset, whence);
    if (-1 == value)
    {
        perror("fseek error");
        return(-1);
    }
    return(value);
}

unsigned int filesize(FILE *fp)
{
        int fSet, fEnd, filelen;
        i_fseek(fp, 0, SEEK_SET);
        fSet = ftell(fp);
        i_fseek(fp, 0, SEEK_END);
        fEnd = ftell(fp);
        rewind(fp);
        return (filelen = fEnd - fSet);
}

int i_list_files(FILE *fp)
{
    struct file tmpfile;
	printf("list Files!\n");
    unsigned int length = filesize(fp);
    unsigned int filecount = FL_LEN;
    int offset = 0;
	 while (filecount < length)
    {
        memset((struct file *)&tmpfile, 0, FL_LEN);
        fseek(fp, filecount, SEEK_SET);
        offset = fread(&tmpfile, sizeof(char), FL_LEN, fp);
        filecount += offset;
        printf("Id = %d,  Owner= %d, Name = %s\n", tmpfile.id, tmpfile.owner, tmpfile.name);
    }
	return(EXIT_SUCCESS);
}

int i_findstring(const char *string, int stringSize, int fd) {
    //printf("string = %s, size = %d\n", string, stringSize);
    int i = 0, j, end;
    char * part = (char *)calloc(stringSize, sizeof(char));
    FILE *fp =NULL;
    fp = fdopen(fd, "r");
    off_t offset = i_lseek(fd, 0L, SEEK_END);
    end = ftell(fp) - stringSize + 2;

    while(i < end) {
        j = 0;
        fseek(fp, (long)i++, SEEK_SET);
        fgets(part, stringSize, fp);
        while(*part) {
            if(*string == *part) {
                j++;
                string++;
                part++;
                continue;
            }
            break;
        }
        if(j == stringSize - 1) {
            return 1;
        } else {
            string -= j;
            part -= j;
        }
    }
    free(part);
    return 0;
}

int i_open(const char *pathname, int flags, mode_t modes)
{
    int fd;
    if ((fd = open(pathname, flags, modes)) == -1)
    {
        perror("open_failed");
        exit(1);
    }

    return (fd);
}

int i_read(int fd, void *msg, int len)
{
    int pos = read(fd, msg, len);
    if(-1 == pos)
    {
        perror("i_read error");
        exit(1);
    }
    return(pos);
}
int i_write(int fd, void *msg, int len)
{
    int pos = write(fd, msg, len);
    if (-1 == pos)
    {
        perror("i_write error");
        exit(1);
    }
    return(pos);
}

int i_listFiles(int fd)
{
    struct file curFile;
    int pos ;
    memset((struct file *)&curFile, 0, FL_LEN);
	printf("list Files!\n");

	for(pos = 1; pos <= fileNum; pos++)
	{
        i_lseek(fd, pos*FL_LEN, SEEK_SET);
        i_read(fd, &curFile, FL_LEN);
        printf("Id = %d,  Owner= %d, Name = %s\n", curFile.id, curFile.owner, curFile.name);
	}
	return(0);
}

int i_listPeers(int fd)
{
    struct peer curPeer;
    int pos ;
    memset((struct peer *)&curPeer, 0, sizeof(struct peer));
	printf("list Peers!\n");
	for(pos = 1; pos <= peerNum; pos++)
	{
        i_lseek(fd, pos*sizeof(struct peer), SEEK_SET);
        i_read(fd, &curPeer, PEER_LEN);
        printf("Peer Name = %s, Id= %d\n", curPeer.name, curPeer.id);
	}
	return(0);
}
/* init a socket,file and server addr */
int i_init()
{
    sockfd = i_socket(AF_INET, SOCK_DGRAM, 0);

    /* initialize server address */
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(SEVR_PORT);     //8081
    // accept connection through eh0, eh1, lo, etc
    server.sin_addr.s_addr = htonl (INADDR_ANY);

    const int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) == -1)
    {
       perror("setsockopt");
       exit(1);
    }

    return (0);
}

char *i_get_time()
{
    time_t time_now;
    time(&time_now);

    return(ctime(&time_now));
}
/* common functions end */
#endif
