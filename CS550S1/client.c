/*
 *title: client.c
 *start_time: 03.Sep.2015
 *end_time: 21.Sep.2015
 */

#include "common.h"
#include <netdb.h>
#define START_PORT 8089

struct sockaddr_in my_addr;
int my_id;
int child = 0;  // for exit system
int parent = 0; // for exit system

/* start:initialize functions */
int get_max_port()
{
    int port = START_PORT;
    int ret = 0;
    int opt = 0;
    int sock = -1;
    sock = i_socket (PF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
    {
        perror("get max port error!");
        return(EXIT_FAILURE);
    }
    do
    {
        port++;
        my_addr.sin_port = htons(port);
        ret = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
        if (ret != 0)
            continue;
        ret = bind(sock, (struct sockaddr*)&my_addr, sizeof(struct sockaddr));
        if (ret != 0)
            continue;
        else
        {
            close(sock);
            return port;
        }
    } while (errno == EADDRINUSE);
    return (EXIT_SUCCESS);
}

/* start:initialize */
int initialize(struct hostent* he)
{
    //struct ifreq req;
    struct sockaddr_in *host;
    unsigned short port;

    /* init server address */
    i_init();
    server.sin_addr = *((struct in_addr *)he->h_addr);

    /* get local ip address
    strcpy(req.ifr_name, "eth1");
    if ( ioctl(sockfd, SIOCGIFADDR, &req) < 0 )
    {
        perror("get local ip error");
        exit(1);
    }
    */
    //host = (struct sockaddr_in*)&(req.ifr_addr);
    //printf("ip: %s\n", inet_ntoa(host->sin_addr));
    //memcpy(&my_addr, (struct sockaddr_in*)&(req.ifr_addr), sizeof(struct sockaddr_in));

    bzero(&my_addr, sizeof(struct sockaddr));
    my_addr.sin_family = AF_INET;
    port = get_max_port();
    int opt = 0;
    setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = htonl (INADDR_ANY);    // accept connection through eh0, eh1, lo, etc
    i_bind(sockfd, (struct sockaddr*)&my_addr, ADDR_LEN);
    //printf("port = %u----%u \n", port,  my_addr.sin_port);


    printf("init successful!!!\n");
    return(EXIT_SUCCESS);
}
/* end:initialize */
/* end:initialize functions */

/* start:exit_sys*/
void exit_system()
{
    close(sockfd);
    kill(child, SIGABRT);
    kill(parent, SIGABRT);
    //system("ps -ef | grep \"./client\" | awk -F\" \" \'{print $2}\' | xargs kill -9");
    exit(0);
}
/* end:exit_sys*/

/* start:small tools*/
int get_input(char *command)
{
    printf(">>");
    scanf(" %c", command);
    return(EXIT_SUCCESS);
}

void clear()
{
    while(getchar()!='\n');
}
/* end:small tools*/

/* start:menu*/
int print_menu()
{
    printf("\n--------------help--menu----------------\n");
    printf("\t h--help munu\n");
    printf("\t c--copy file\n");
    printf("\t f--share file\n");
    printf("\t q--query files\n");
    printf("\t s--send message\n");
    printf("\t e--exit the system\n");
    printf("----------------help--menu----------------\n");
    return(EXIT_SUCCESS);
}

int menu()
{
    /* to avoid the output at mixed with the sub process */
    sleep(1);
    print_menu();
    char cmd;
    while (0 == get_input(&cmd))
    {
        switch(cmd)
        {
            case 'h':
                print_menu();
                break;
            case 's':
                send_message();
                break;
            case 'f':
                share_file();
                break;
            case 'q':
                search_file();
                break;
            case 'c':
                get_file_creater();
                break;
            case 'e':
                exit_system();
                break;
            default :
                break;
        }
    }
    return(EXIT_SUCCESS);
}
/* end:menu*/

/* start:message contol*/

/* start:message contol :share file and share_msg_recv */
int share_file()
{
    struct msg the_msg;
    memset((struct msg*)&the_msg, 0, sizeof(the_msg));
    char end = '@';

    printf("input file absolute path:");
    #ifdef TEST
    int i ;
    for (i = 1; i<= 2000; i++ )
    {
        usleep(10);
        sprintf(the_msg.content, "/home/fei/Dropbox/CS550/CS550S1/peer%d/radom%d", my_id, i);
        printf("filepath=%s\n", the_msg.content);

        if (strlen(the_msg.content) != 0)
        {
            the_msg.flag = 4;
            the_msg.id_from = my_id;
            the_msg.id_to = SEVR_ID;

            i_sendto(sockfd, &the_msg,  MSG_LEN, 0, (struct sockaddr*)&server, ADDR_LEN);
        }
    }
    usleep(100);
    #endif
    scanf("%s", the_msg.content);
    if (strlen(the_msg.content) != 0)
    {
        the_msg.flag = 4;
        the_msg.id_from = my_id;
        the_msg.id_to = SEVR_ID;

        i_sendto(sockfd, &the_msg,  MSG_LEN, 0, (struct sockaddr*)&server, ADDR_LEN);
        return(EXIT_SUCCESS);
    }
    else
    {
        perror("share file error!");
        return(EXIT_FAILURE);
    }
    return(EXIT_SUCCESS);
}

int share_file_recv(struct msg *pmsg)
{
    char time_info[25];

    /* handle the msg */
    if (pmsg->flag == 4)
    {
        printf("%s:Peer %d shared file: %s success.\n", i_get_time(), my_id, pmsg->content);
    }
    else
    {
        printf("%s:Peer %d shared file: %s failed.\n", i_get_time(), my_id, pmsg->content);
    }
    return(EXIT_SUCCESS);
}
/* end:message contol :share file and share_msg_recv */

/* start:message contol :send_msg and recv_msg */
int send_message()
{
    int id = 0;
    struct msg the_msg;
    char end = '@';

    printf("input receiver id:");
    scanf("%d", &id);
    getchar();     //remove return char after scanf
    printf("\ninput content:");
    bzero(&the_msg, MSG_LEN);
    i_input(the_msg.content);

    if (strlen(the_msg.content) != 0)
    {
        the_msg.flag = 3;
        the_msg.id_from = my_id;
        the_msg.id_to = id;

        i_sendto(sockfd, &the_msg, sizeof(struct msg), 0, (struct sockaddr*)&server, ADDR_LEN);
        printf("%s:send message to id:%d success.\n", i_get_time(), id);
        return(EXIT_SUCCESS);
    }
    else
    {
        perror("send message error!");
        return(EXIT_FAILURE);
    }
}

int chat_msg_recv(struct msg *pmsg)
{
    char time_info[25];
    char end_symbol = '&';

    /* handle the msg */
    printf("%s:Message:from %s (id:%d) to U:\n%s\n", i_get_time(), pmsg->append, pmsg->id_from, pmsg->content);

    return(EXIT_SUCCESS);
}

/* end:message contol :send_msg and recv_msg */

/* start:message contol :search file and file_list_recv */
int search_file()
{
    char name[11];
    memset(name, 0, sizeof(name));
    printf("input your query string(less than 10 chars):");
    scanf("%s", name);
    //printf("%s\n", name);
    struct msg query_msg;
    //memset((struct msg *)&query_msg, 0, sizeof(struct msg));
    bzero(&query_msg, MSG_LEN);
    query_msg.flag = 5;
    query_msg.id_from = my_id;
    query_msg.id_to = 0;
    strcpy(query_msg.content, name);
    printf("%s\n", query_msg.content);
    #ifdef TEST
    int count;
    for (count = 0; count <1000; count++)
    {
        usleep(100);
        i_sendto(sockfd, (struct msg*)&query_msg, MSG_LEN, 0, (struct sockaddr*)&server,  ADDR_LEN);
    }
    #endif
    i_sendto(sockfd, (struct msg*)&query_msg, MSG_LEN, 0, (struct sockaddr*)&server,  ADDR_LEN);
    printf("wating for server reply...\n");
    return(EXIT_SUCCESS);
}

int file_list_recv(struct msg *pmsg)
{
    char time_info[25];
    char end_symbol [3]= "&$\0";
    char segment[100] = {0};
    /* handle the msg */
    if (-5 == pmsg->flag)
    {
        printf("%s:file from %s (id:%d) to U:\n%s\n", i_get_time(), pmsg->append, pmsg->id_from, "No matching results found.");
        return(EXIT_FAILURE);
    }
    else
    {
        printf("%s:file from %s (id:%d) to U:\n",  i_get_time(), pmsg->append, pmsg->id_from);
        /* Establish string and get the first token: */
        char *token = strtok(pmsg->content,end_symbol);
        while ( NULL != token)
        {
            /* While there are tokens in "string" */
            printf( "%s\n", token );
            /* Get next token: */
            token = strtok( NULL, end_symbol);
        }
        //printf("\t%s", i_get_time());
        return(EXIT_SUCCESS);
    }
}
/* end:message contol :search file and file_list_recv */

/* start:message contol :get file creater and peer_addr_recv */
int get_file_creater()
{
    int fileNo = 0;
    printf("input the file id:");
    scanf("%d", &fileNo);
    struct msg creater_msg;
    //char cfileId[2];
    //sprintf(cfileId, "%d", fileId);
    bzero(&creater_msg, MSG_LEN);
    creater_msg.flag = 6;
    creater_msg.id_from = my_id;
    creater_msg.id_to = SEVR_ID;
    bzero(creater_msg.content, CNTNT_LEN);
    sprintf(creater_msg.content, "%d", fileNo);

    i_sendto(sockfd, (struct msg*)&creater_msg, sizeof(struct msg), 0, (struct sockaddr*)&server, ADDR_LEN);
    /* after input msg ,wait for server respond*/
    printf("wating for server reply...\n");
    return(EXIT_SUCCESS);
}

int get_addr_recv(struct msg *pmsg)
{
    //Connect to File's Owner
    struct sockaddr_in to_addr;
    char sToId[3] = {0};
    char sToIP[16] = {0};
    char sToPort[16] = {0};
    char sFilePath[256] = {0};
    sscanf((pmsg->content), "%[^:]:%[^:]:%[^:]:%s", sToId, sToIP, sToPort, sFilePath);
    bzero(&to_addr, sizeof(to_addr));
    to_addr.sin_family = AF_INET;
    inet_pton(AF_INET, sToIP, &to_addr.sin_addr);
    to_addr.sin_port = htons(atoi(sToPort));
    //printf("info: %s %s %d\n", sToId, sToIP, atoi(sToPort));
    //printf("info: %s %s %d\n", sToId, inet_ntoa(((struct sockaddr_in)to_addr).sin_addr), ((struct sockaddr_in)to_addr).sin_port);

    struct msg file_down_msg;
    file_down_msg.flag = 7;
    file_down_msg.id_from = my_id;
    file_down_msg.id_to = atoi(sToId);
    strcpy(file_down_msg.content, sFilePath);
    strcpy(file_down_msg.append, inet_ntoa(my_addr.sin_addr));
    strcat(file_down_msg.append, ":");
    char sPort[16] = {0};
    sprintf(sPort, "%d", ntohs(my_addr.sin_port));
    strcat(file_down_msg.append, sPort);
	//printf("from_addr:%s\n", file_down_msg.append);
    i_sendto(sockfd, (struct msg*)&file_down_msg, MSG_LEN, 0, (struct sockaddr*)&to_addr, ADDR_LEN);
	printf("%s:send file copy request success!\n",i_get_time());
    return(EXIT_SUCCESS);
}

/* start:message contol :send file and receive file function */
int put_file_recv(struct msg *pmsg)
{
    struct file tFile;
    memset(&tFile, 0, FL_LEN);
    strcpy(tFile.name, pmsg->content);

    struct sockaddr_in to_addr;
    char sToIP[16] = {0};
    char sToPort[16] = {0};
    sscanf((pmsg->append), "%[^:]:%s", sToIP, sToPort);
    bzero(&to_addr, sizeof(to_addr));
    to_addr.sin_family = AF_INET;
    inet_pton(AF_INET, sToIP, &to_addr.sin_addr);
    to_addr.sin_port = htons(atoi(sToPort));

    struct msg file_send_msg;
    file_send_msg.flag = 8;
    file_send_msg.id_from = my_id;
    file_send_msg.id_to = pmsg->id_from;
    strcpy(file_send_msg.append, inet_ntoa(my_addr.sin_addr));
    strcat(file_send_msg.append, ":");
    char sPort[16] = {0};
    sprintf(sPort, "%d", ntohs(my_addr.sin_port));
    strcat(file_send_msg.append, sPort);

    FILE *fp = NULL;
    fp = fopen(tFile.name, "rb");
    if (fp ==  NULL)
    {
        tFile.filesize = 0;
        printf("%s:no file can't be downoaded\n", i_get_time());
        memcpy(file_send_msg.content, (unsigned char *)&tFile, FL_LEN);
        i_sendto(sockfd, (struct msg*)&file_send_msg, MSG_LEN, 0, (struct sockaddr*)&to_addr, ADDR_LEN);
        return (EXIT_FAILURE);
    }
    else
    {
        tFile.filesize = filesize(fp);
        printf("%s:file size: %d\n", i_get_time(), tFile.filesize);
        memcpy(file_send_msg.content, (unsigned char *)&tFile, FL_LEN);
        i_sendto(sockfd, (struct msg*)&file_send_msg, MSG_LEN, 0, (struct sockaddr*)&to_addr, ADDR_LEN);
    }
    struct sockaddr back_addr;
    struct msg back_msg;
    bzero(&back_addr, ADDR_LEN);
    bzero(&back_msg, MSG_LEN);
    int len = ADDR_LEN;
    i_recvfrom(sockfd, (struct msg*)&back_msg, MSG_LEN, 0, &back_addr, &len);
    if ( strcmp(pmsg->content,"***END***") == 0)
    {
        printf("%s:Send failed\n", i_get_time());
        return (EXIT_FAILURE);
    }
    printf("%s:Sending %s to Peer%d\n", i_get_time(), tFile.name, file_send_msg.id_to);

    unsigned int filecount = 0;
    int offset = 0;
    int err = 0;
    while (filecount < tFile.filesize)
    {
        memset(file_send_msg.content, 0, CNTNT_LEN);
        offset = tFile.filesize - filecount;
        if (offset > CNTNT_LEN)
            err = fread(file_send_msg.content, sizeof(char), CNTNT_LEN, fp);
        else
            err = fread(file_send_msg.content, sizeof(char), offset, fp);
        //printf("put offset = %d\n", offset);
        filecount += err;
        sprintf(file_send_msg.append, "%d", err);
        i_sendto(sockfd,  (struct msg*)&file_send_msg, MSG_LEN, 0, (struct sockaddr*)&to_addr, ADDR_LEN);
    }
    printf("%s:Send file %s finished!\n", i_get_time(), tFile.name);
    fclose(fp);
    return 0;
}

int get_file_recv(struct msg *pmsg)
{
    //Initialize session
    struct sockaddr_in to_addr;
    char sToIP[16] = {0};
    char sToPort[16] = {0};
    sscanf((pmsg->append), "%[^:]:%s", sToIP, sToPort);
    bzero(&to_addr, sizeof(to_addr));
    to_addr.sin_family = AF_INET;
    inet_pton(AF_INET, sToIP, &to_addr.sin_addr);
    to_addr.sin_port = htons(atoi(sToPort));

    // Check file size
    struct file tFile;
    memset(&tFile, 0, FL_LEN);
    memcpy(&tFile, (struct file *)(pmsg->content), FL_LEN);
    printf("%s:File Size = %d\n", i_get_time(), tFile.filesize);
    if (tFile.filesize == 0)
    {
        printf("The file %s does not exist,can't be downoad\n", tFile.name);
        return (EXIT_FAILURE);
    }

    //Save to ./Peer%id folder
    char sPath[256] = {0};
    sprintf(sPath, "./peer%d", my_id);
    if (-1 == access(sPath, X_OK))
    {
        int status = mkdir(sPath, S_IRWXU | S_IRWXG | S_IRWXO); /*777*/
        (!status) ? (printf("Directory created %s\n", sPath)) : (printf("Unable to create directory %s\n", sPath));
    }
	char *ptr, r = '/';
	ptr = strrchr(tFile.name, r);
	strcat(sPath, ptr);
	FILE *fp = fopen(sPath, "wb");
	printf("%s:File Full Path = %s\n", i_get_time(), sPath);

    // Send the checking result for preparing file transfer
	struct msg file_send_msg;
    memset((struct msg*)&file_send_msg, 0, sizeof(file_send_msg));
    if (fp == NULL)
    {
        printf("the file exists!\n");
        strcpy(file_send_msg.content, "***END***");
        i_sendto(sockfd,  (struct msg*)&file_send_msg, MSG_LEN, 0, (struct sockaddr*)&to_addr, ADDR_LEN);
        return (EXIT_FAILURE);
    }
    else
    {
        strcpy(file_send_msg.content, "***ok***");
        i_sendto(sockfd,  (struct msg*)&file_send_msg, MSG_LEN, 0, (struct sockaddr*)&to_addr, ADDR_LEN);
    }

    //Download the File
    printf("%s:Downloading %s from peer %d\n", i_get_time(), tFile.name, pmsg->id_from);
    int filecount = 0;
    struct sockaddr back_addr;
    struct msg back_msg;
    bzero(&back_addr, ADDR_LEN);
    bzero(&back_msg, MSG_LEN);
	int err;
	int len = ADDR_LEN;
    while (filecount < tFile.filesize)
    {
        i_recvfrom(sockfd, (struct msg*)&back_msg, MSG_LEN, 0, &back_addr, &len);
        err = atoi(back_msg.append);
        //printf("get err = %d\n", err);
        fwrite(back_msg.content, err, 1, fp);
        filecount+=err;
        //printf("get offset = %d\n", filecount);
    }
    fclose(fp);
    printf("%s:%u bytes transferred\n",i_get_time(), filecount);
    printf("Download finish!\n");
    return (EXIT_SUCCESS);
}
/* end:message contol :send file and receive file function */
/* end:message contol*/

int handle_msg(struct msg *pmsg)
{
    switch (pmsg->flag)
    {
        case 3:
        case -3:
            chat_msg_recv(pmsg);
            break;
        case 4:
        case -4:
            // share
            share_file_recv(pmsg);
            break;
        case 5:
        case -5:
            file_list_recv(pmsg);
            break;
        case 6:
        case -6:
            get_addr_recv(pmsg);
            break;
        case 7:
        case -7:
            put_file_recv(pmsg);
            break;
        case 8:
        case -8:
            get_file_recv(pmsg);
            break;
        default:
            break;
    }
    return(0);
}

int listen_msg()
{
    struct msg msg_recv;
    struct sockaddr addr_recv;
    int len = ADDR_LEN;
    printf("begin listening...\n");

    for ( ; ; )
    {
        i_recvfrom(sockfd, &msg_recv, MSG_LEN, 0, &addr_recv, &len);
        handle_msg(&msg_recv);
    }
}

/* start:log in and register */
int login_peer()
{
    /* input id:*/
    printf("*****logon>>\n");
    printf("id:");
    scanf("%d", &my_id);
    getchar();
    /* send login information */
    struct msg log_msg;

    bzero(&log_msg, MSG_LEN);
    log_msg.flag = 2;
    log_msg.id_from = my_id;
    log_msg.id_to = 0;

    i_sendto(sockfd, (struct msg*)&log_msg, MSG_LEN, 0, (struct sockaddr*)&server, sizeof(struct sockaddr));
    struct msg msg_recv;
    struct sockaddr addr_recv;
    int len = ADDR_LEN;
    i_recvfrom(sockfd, &msg_recv, MSG_LEN, 0, &addr_recv, &len);
    if (2 == msg_recv.flag)
    {
		char sToIP[16] = {0};
		char sToPort[16] = {0};
		sscanf(msg_recv.append, "%[^:]:%s", sToIP, sToPort);
		inet_pton(AF_INET, sToIP, (struct sockaddr_in*)&my_addr.sin_addr);

        printf("login success\n");
        return(EXIT_FAILURE);
    }
    else
    {
        printf("login error:%s\n", msg_recv.content);
        printf("please relog..\n");
        my_logon();
    }

    return (EXIT_SUCCESS);
}

int register_peer()
{
    printf("*****register>>\n");
    /* input chat name */
    char name[10];
    bzero(name, sizeof(name));
    printf("input your name(less 10 char):");
    scanf(" %s", name);

    /* send register information*/
    struct msg reg_msg;

    bzero(&reg_msg, MSG_LEN);
    reg_msg.flag = 1;
    reg_msg.id_from = 0;
    reg_msg.id_to = SEVR_ID;
    bzero(reg_msg.content, CNTNT_LEN);
    strncpy(reg_msg.content, name, 10);
    reg_msg.content[10] = '\n';

    /* send regist informatin to server */
    i_sendto(sockfd, (struct msg*)&reg_msg, sizeof(struct msg), 0,  (struct sockaddr*)&server, ADDR_LEN);
    /* after input msg ,wait for server respond*/
    printf("wating for server reply...\n");
    struct msg msg_recv;
    struct sockaddr addr_recv;
    int len = ADDR_LEN;
    i_recvfrom(sockfd, &msg_recv, MSG_LEN, 0, &addr_recv, &len);

    /* check whether pass */
    if (1 != msg_recv.flag)
    {
        perror("register failed!\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        my_id = msg_recv.id_to;
        printf("%s:you have name:%s(id:%d) success.\n", i_get_time(), msg_recv.content, msg_recv.id_to);
        my_logon();
        return(EXIT_SUCCESS);
    }
}
/* end:log in and register */

int my_logon()
{
    /* choose logon or register*/
    while (1)   {
        printf("are you want login(l) or register(r) or exit(e) \n");
        char flogreg = '0';
        get_input(&flogreg);
        if ('l' == flogreg) {
                login_peer();break;
        }
        else if ( 'r' == flogreg) {
                register_peer();break;
        }
        else if ('e' == flogreg) {
                exit_system();break;
        }
    }
    return (0);
}
/* end:logon */

#ifdef TEST
int get_file_name(int fileId, char *fileName)
{
    struct msg file_msg;
    char cfileId[2];
    sprintf(cfileId, "%d", fileId);
    bzero(&file_msg, MSG_LEN);
    file_msg.flag = 7;
    file_msg.id_from = my_id;
    file_msg.id_to = SEVR_ID;
    bzero(file_msg.content, CNTNT_LEN);
    strncpy(file_msg.content, cfileId, 1);
    file_msg.content[1] = '\n';

    i_sendto(sockfd, (struct msg*)&file_msg, sizeof(struct msg), 0, (struct sockaddr*)&server, ADDR_LEN);
    //after input msg ,wait for server respond
    printf("wating for server reply...\n");

    struct sockaddr in_addr;
    struct msg msg_back;
    int len = ADDR_LEN;

    bzero(&in_addr, ADDR_LEN);
    bzero(&msg_back, MSG_LEN);
    i_recvfrom(sockfd,(struct msg*)&msg_back, MSG_LEN,0, &in_addr, &len);

    // check whether pass
    if (7 == msg_back.flag)
    {
        strcpy(fileName, msg_back.content);
        printf("%s:File Name = %s", i_get_time(), msg_back.content);
        return (EXIT_SUCCESS);
    }
    else
    {
        perror(msg_back.content);
        return (EXIT_FAILURE);
    }
}
#endif // TEST

int main(int argc, char *argv[])
{
    struct hostent *he;
    pid_t pid;

    if (argc !=2) {
        printf("Usage: %s <IP Address>\n",argv[0]);
        exit(1);
    }

    if ((he=gethostbyname(argv[1]))==NULL){ /* calls gethostbyname() */
        printf("gethostbyname() error\n");
        exit(1);
    }

    initialize(he);
    printf("\n************welcome!************\n");
    my_logon();

    switch (pid = fork())
    {
        case -1 :
            perror("fork error!\n");
            exit(1);
            break;
        case 0 :
            child = getpid();
            listen_msg();
            break;
        default :
            parent = getpid();
            menu();
            break;
    }
}
