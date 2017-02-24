/*
 *title:server.c
 *content:server part
 *start time: Sep.05.2015
 *end time:  Sep.20 2015
 */

#include "common.h"
#include <sys/time.h>   // for performance testing
int file_list_fd,peer_cfg_fd;
FILE *fpg, *ffg;
int child = 0;  // for exit system
int parent = 0; // for exit system
int scht = 0;   // for performance testing
int regt = 0;   // for performance testing
struct timeval start,stop,diff;     // for performance testing
/* start:initialization */
int initialize()
{
	i_init();
	mode_t f_attrib;    // Type of file attribute bitmasks
    f_attrib = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
	file_list_fd = i_open("./filelist.cfg", O_RDWR|O_CREAT,f_attrib);
	peer_cfg_fd = i_open("./peer.cfg", O_RDWR|O_CREAT,f_attrib);

	ffg = fopen("./filelist.cfg", "r+");        // r+ = write + read
	fpg = fopen("./peer.cfg", "r+");
	struct peer per;
	struct file flst;
    off_t offset = 0;
	int number = 0;
	/* init the user list file's fist user to 0*/
	memset((struct peer*)&per, 0, sizeof(struct peer));
	memset((struct file*)&flst, 0, FL_LEN);
    if(fpg == NULL || ffg == NULL)
    {
        perror("open config file error");
        return -1;
    }

	i_lseek(file_list_fd, 0, SEEK_SET);
	i_write(file_list_fd, (char*)&flst, FL_LEN);
	offset = i_lseek(file_list_fd, -FL_LEN, SEEK_END);
	fileNum = offset / (FL_LEN);
    //printf("There are %d shared files exist.\n", fileNum);

	/* bind the struct sockaddr_in server to the sockfd */
	i_bind(sockfd, (struct sockaddr*)&server, ADDR_LEN);

	return(0);
}
/* end:initialization */


int set_peer_cfg(int peerid, char *item, char *value)
{
    /*
    *linebuffer：read one line to buffer
    *buffer1：1st segment in thel line
    *buffer2：2nd segment in thel line
    *buffer3：3rd segment in thel line
    *buffer4：4th segment in thel line
    */
    char linebuffer[150] = {0};
    char buffer1[32] = {0};
    char buffer2[32] = {0};
    char buffer3[32] = {0};
    char buffer4[32] = {0};
    char tmp[32] = {0};
    int line_len = 0;
    int len = 0;
    int res;
    int bFound = 0;

    while(!feof(fpg))
    {
        fgets(linebuffer, 150, fpg);
        line_len = strlen(linebuffer);
        len += line_len;
        /*
        * buffer1=id
        * buffer2=1
        * buffer3=state / ip / port
        * buffer4=1 /255.255.255.255/ 22222
        */
        sscanf(linebuffer, "%[^:]:%[^:]:%[^:]:%s", buffer1, buffer2, buffer3, buffer4);
        #ifdef DEBUG
        printf("buffer1 %s %d\n", buffer1, strlen(buffer1));
        printf("buffer2 %s %d\n", buffer2, strlen(buffer2));
        printf("buffer3 %s %d\n", buffer3, strlen(buffer3));
        printf("buffer4 %s %d\n", buffer4, strlen(buffer4));
        #endif // DEBUG
        sprintf(tmp, "%30s", item);

        if(peerid == atoi(buffer2) && !strcmp(buffer3, tmp))
        {
            // found the item header
            bFound = 1;
            len -= strlen(linebuffer);
            // compute offset and locate
            len += strlen(buffer1) + strlen(buffer2)  + strlen(buffer3) + 3;
            res = fseek(fpg, len, SEEK_SET);
            if(res < 0)
            {
                perror("fseek");
                rewind(fpg);
                return -1;
            }
            //printf("%32s\n", item);
            //Upgrade new value
            fprintf(fpg, "%30s\n", value);
            rewind(fpg);
            return (EXIT_SUCCESS);
        }
    }
    if (0 == bFound)
    {
        /* Write new line */
        res = fseek(fpg, 0, SEEK_END);
        //printf("write new line %d %s %s\n", peerid, item, value);
        fprintf(fpg, "id:");
        fprintf(fpg, "%30d:", peerid);
        fprintf(fpg, "%30s:", item);
        fprintf(fpg, "%30s\n", value);
    }
    rewind(fpg);
    return (EXIT_SUCCESS);
}

int get_peer_maxid()
{
    char linebuffer[150] = {0};
    fgets(linebuffer, 150, fpg);
    int line_len = strlen(linebuffer);
    fseek(fpg, -line_len, SEEK_END);
    fgets(linebuffer, 150, fpg);
    char buffer1[32] = {0};
    char buffer2[32] = {0};
    sscanf(linebuffer, "%[^:]:%[^:]", buffer1, buffer2);
    int nId = atoi(buffer2);
    rewind(fpg);
    return nId;
}

int get_peer_cfg(int peerid, char *item, char *value)
{
    /*
    *linebuffer：read one line to buffer
    *buffer1：1st segment in thel line
    *buffer2：2nd segment in thel line
    *buffer3：3rd segment in thel line
    *buffer4：4th segment in thel line
    */
    char linebuffer[150] = {0};
    char buffer1[32] = {0};
    char buffer2[32] = {0};
    char buffer3[32] = {0};
    char buffer4[32] = {0};
    char tmp[32] = {0};
    int line_len = 0;
    int len = 0;
    int res;

    rewind(fpg);
    while(fgets(linebuffer, 150, fpg))
    {
        line_len = strlen(linebuffer);
        len += line_len;
        /*
        * buffer1=id
        * buffer2=1
        * buffer3=name/ state / ip / port
        * buffer4=peername1 1 /255.255.255.255/ 22222
        */
        sscanf(linebuffer, "%[^:]:%[^:]:%[^:]:%s", buffer1, buffer2, buffer3, buffer4);
        //printf("item %s\n", item);
        sprintf(tmp, "%30s", item);
        if(peerid == atoi(buffer2) && !strcmp(buffer3, tmp))
        {
            // found the item and copy
            strcpy(value, buffer4);
            rewind(fpg);
            return (EXIT_SUCCESS);
        }
    }
    strcpy(value, "\0");
    rewind(fpg);
    return (EXIT_FAILURE);
}

/* read the peer configure file and get the peer ip address and port */
int get_peer_addr(int peerId, char * addrprt)
{
    if (peerId > 0)
    {
        char peerIP[16] = {0};
        char peerPort[16] = {0};
        get_peer_cfg(peerId, "ip", peerIP);
        get_peer_cfg(peerId, "port", peerPort);
        sprintf(addrprt, "%d", peerId);
        strcat(addrprt, ":");
        strcat(addrprt, peerIP);
        strcat(addrprt, ":");
        strcat(addrprt, peerPort);
        return (EXIT_SUCCESS);
    }
    return (EXIT_FAILURE);
}

/* start:message control */
int create_user(struct msg *msg_recv, struct sockaddr *addr)
{
	struct peer per;
	//printf("a registration request come:\n");

	/* find the last user and hava the place to add a new user */
	int n;
    char name[11];
	strncpy(name, msg_recv->content, 10);
    name[10] = '\n';
    n = get_peer_maxid();
    char sda[INET_ADDRSTRLEN];
	if ( -1 == n || 0 == n)
        n = 1;
    else
        n = n+1;

    char sIp[32] = {0};
    unsigned short nPort = 0;
    char sPort[16] = {0};
    strcpy(sIp, inet_ntoa(((struct sockaddr_in *)addr)->sin_addr));
    nPort = ntohs(((struct sockaddr_in *)addr)->sin_port);
    sprintf(sPort, "%u", nPort);
    int result = set_peer_cfg(n, "name", name);
    result += set_peer_cfg(n, "ip", sIp);
    result += set_peer_cfg(n, "port", sPort);
    result += set_peer_cfg(n, "state", "0");

    if (result == 0)
    {
        msg_recv->flag = 1;
        msg_recv->id_from =SEVR_ID;
        msg_recv->id_to = n;

        /* regist to the user list then tell the user reg success */
        i_sendto(sockfd, (struct msg*)msg_recv, sizeof(struct msg), 0, addr, ADDR_LEN);
        printf("%s:Id:%d Name:%s regist sucess!\n", i_get_time(), n, name);
        return(EXIT_SUCCESS);
    }
    else
    {
        msg_recv->flag = -1; //register failed.
        strcpy(msg_recv->content, "register failed!");
        i_sendto(sockfd, (struct msg*)msg_recv, sizeof(struct msg), 0, addr, ADDR_LEN);
        printf("%s:Name:%s regist failed!\n", i_get_time(), name);
        return(EXIT_FAILURE);
    }
}

int check_login(struct msg *msg_recv, struct sockaddr *addr)
{
	int i = msg_recv->id_from;

	/* a login requet */
	printf("a login request come!\n");

	int nBack = 0;
    char name[11] = {0};
    get_peer_cfg(i, "name", name);
    char sState[17] = {0};
    get_peer_cfg(i, "state", sState);

	//printf("name = %s\n", name);
    if (0 != strlen(name))
    {
        //printf("state=%s\n", sState);
        if (strcmp("0", sState) == 0 || strlen(sState) == 0)
        {
            /* tell user pass */
            char sIp[32] = {0};
            unsigned short nPort = 0;
            char sPort[16] = {0};
            int result = 0;
            strcpy(sIp, inet_ntoa(((struct sockaddr_in *)addr)->sin_addr));
            nPort = ntohs(((struct sockaddr_in *)addr)->sin_port);
            sprintf(sPort, "%u", nPort);
            result += set_peer_cfg(i, "ip", sIp);
            result += set_peer_cfg(i, "port", sPort);
            result += set_peer_cfg(i, "state", "1");
            if (0 ==result)
            {
                msg_recv->flag = 2;
				strcpy(msg_recv->append, sIp);
				strcat(msg_recv->append, ":");
				strcat(msg_recv->append, sPort);
                i_sendto(sockfd, (struct msg*)msg_recv, sizeof(struct msg), 0, addr, ADDR_LEN);
                printf("%s:Id %d login success!\n", i_get_time(), i);
                return(0);
            }
            else
                nBack = 1;
        }
        else if (strcmp("1", sState) == 0)
        {
            msg_recv->flag = -2;
            strcpy(msg_recv->content, "Already login!\n");
            i_sendto(sockfd, (struct msg*)msg_recv, sizeof(struct msg), 0, addr, ADDR_LEN);
            printf("%s:Id %d login failed!\n", i_get_time(), i);
            return(0);
        }
    }
    else
        nBack = 1;
    if (1 == nBack)
    {
		/* login failed */
        printf("%s:Id %d login failed!\n", i_get_time(), i);
		bzero(msg_recv->content, CNTNT_LEN);
		msg_recv->flag = -2;
        i_sendto(sockfd, (struct msg*)msg_recv, sizeof(struct msg), 0, addr, ADDR_LEN);
        return(-1);
	}
    return (EXIT_SUCCESS);

}

int send_msg(struct msg *msg_recv, struct sockaddr *addr)
{
	/* a common message come */
	printf("a ordinary message come !\n");

	int i = msg_recv->id_to;
    char sPeerAddrprt[32] = {0};
    get_peer_addr(i, sPeerAddrprt);
	//printf("PeerAddr = %s\n", sPeerAddrprt);
    struct sockaddr_in to_addr;
    char sToId[3] = {0};
    char sToIP[16] = {0};
    char sToPort[16] = {0};
    sscanf(sPeerAddrprt, "%[^:]:%[^:]:%s", sToId, sToIP, sToPort);
    bzero(&to_addr, sizeof(to_addr));
    to_addr.sin_family = AF_INET;
    inet_pton(AF_INET, sToIP, &to_addr.sin_addr);
    to_addr.sin_port = htons(atoi(sToPort));
    //printf("info: %s %s %d\n", sToId, sToIP, atoi(sToPort));

	i_sendto(sockfd, msg_recv, MSG_LEN, 0,   (struct sockaddr*)&to_addr, ADDR_LEN);
	printf("%s:id%d send a message to id%d sucess!\n", i_get_time(), msg_recv->id_from, msg_recv->id_to);

	return(EXIT_SUCCESS);
}

int register_file(struct msg *msg_recv, struct sockaddr *addr)
{
	struct file flst;
    #ifdef TEST
    regt++;
    if (regt == 1)
        gettimeofday(&start,0);
    #endif //TEST
	//printf("a file register requet come:\n");

	/* find the last  and the place to add a new user */
	int n;
	i_lseek(file_list_fd, -FL_LEN, SEEK_END);
	i_read(file_list_fd, &flst, FL_LEN);

    int count = 0;
    const char *name;
	name = &(msg_recv->content[0]);
	strcpy((flst.name), name);

	flst.id = (flst.id + 1);
	flst.owner = msg_recv->id_from;
	msg_recv->id_from = SEVR_ID;
    msg_recv->id_to = flst.owner;
    strncpy(msg_recv->append, "Server", 10);
    //printf("Idfrom = %d Idto = %d append = %s\n", msg_recv->id_from, msg_recv->id_to, msg_recv->append);
	i_lseek(file_list_fd, 0, SEEK_END);
	if (1 < i_write(file_list_fd, &flst, FL_LEN))
	{
        /* tell the user share file successfully */
        msg_recv->flag = 4;
        i_sendto(sockfd, (struct msg*)msg_recv, sizeof(struct msg), 0, addr, ADDR_LEN);
        printf("%s:Id %d File %s register success!\n", i_get_time(), flst.id, flst.name);
        fileNum = flst.id;
        #ifdef TEST
        gettimeofday(&stop,0);
        timeval_subtract(&diff,&start,&stop);   
        printf("register file %d times in %d microseconds\n", regt, diff.tv_usec);
        #endif //TEST
        return(EXIT_SUCCESS);
	}
	else
	{
        /* tell the user share file failed */
        msg_recv->flag = -4;
        i_sendto(sockfd, (struct msg*)msg_recv, sizeof(struct msg), 0, addr, ADDR_LEN);
        printf("%s:Id %d File %s register failed!\n", i_get_time(), flst.id, flst.name);
        return(EXIT_FAILURE);
	}
}

#ifdef TEST
int timeval_subtract(struct timeval* result, struct timeval* x, struct timeval* y)   
{
        int   nsec;   
    
        if   (   x->tv_sec>y->tv_sec   )   
                  return   -1;   
    
        if   (   (x->tv_sec==y->tv_sec)   &&   (x->tv_usec>y->tv_usec)   )   
                  return   -1;   
    
        result->tv_sec   =   (   y->tv_sec-x->tv_sec   );   
        result->tv_usec   =   (   y->tv_usec-x->tv_usec   );   
    
        if   (result->tv_usec<0)   
        {   
                  result->tv_sec--;   
                  result->tv_usec+=1000000;   
        }   
    
        return   0;   
}
#endif //TEST

int search_file(struct msg *msg_recv, struct sockaddr *addr)
{
    #ifdef TEST
    scht++;
    if (scht == 1)
        gettimeofday(&start,0);
    #endif //TEST
	//printf("search File!\n");
	char keyword[11] = "";
	char segment[100] = "";
	strncpy(keyword, msg_recv->content, 10);
	memset(msg_recv->content, 0, CNTNT_LEN);
	//printf("keyword1 = %s\n", keyword);

    struct file tmpFile;
    //struct msg tmpMsg;
    int posi;
    int cursor = 0;
    memset((struct file *)&tmpFile, 0, FL_LEN);

    //memset((struct msg *)&tmpMsg, 0, sizeof(struct msg));
	//printf("keyword2 = %s\n", keyword);
	for(posi = 1; posi <= fileNum; posi++)
	{
        //printf("keyword3 = %s\n", keyword);
        i_lseek(file_list_fd, posi*FL_LEN, SEEK_SET);
        i_read(file_list_fd, &tmpFile, FL_LEN);

        sprintf(segment, "FileID= %d PeerID= %d Name= %s", tmpFile.id, tmpFile.owner, tmpFile.name);
        //printf("segment = %s\n", segment);
        if (0 != strstr(tmpFile.name, keyword)) {
            //strcpy(msg_recv->content[cursor*sizeof(segment)], segment);
            //strncpy(&(msg_recv->content[cursor*sizeof(segment)]), segment, sizeof(segment));
            if(cursor > 0){
                strncat(msg_recv->content, "$", 1);
            }
            if (CNTNT_LEN - strlen(msg_recv->content) > strlen(segment))
                strncat(msg_recv->content, segment, strlen(segment));
            else
                break;
            //printf("msg_recv.content = %s\n", msg_recv->content);
            memset(segment, 0, sizeof(segment));
            cursor++;
        }
	}
    strncat(msg_recv->content, "&", 1);
    strcpy(msg_recv->append, "SERVER");
	if (cursor > 0) {
        msg_recv->flag = 5;
    }
    else
        msg_recv->flag = -5;
    msg_recv->id_to = msg_recv->id_from;
    msg_recv->id_from = SEVR_ID;
    i_sendto(sockfd, (struct msg*)msg_recv, sizeof(struct msg), 0, addr, ADDR_LEN);
	printf("%s:matched files information send success!\n",i_get_time());
    
    #ifdef TEST
    gettimeofday(&stop,0);
    timeval_subtract(&diff,&start,&stop);   
    printf("search file %d times in %d microseconds\n", scht, diff.tv_usec);
    #endif //TEST
    return(0);
}

int get_peer_id(int fileId)
{
    struct file tmpFile;
    int posi;
    int cursor = 0;
    memset((struct file *)&tmpFile, 0, FL_LEN);

	for(posi = 1; posi <= fileNum; posi++)
	{
        i_lseek(file_list_fd, posi*FL_LEN, SEEK_SET);
        i_read(file_list_fd, &tmpFile, FL_LEN);

        if (tmpFile.id == fileId) {
			printf("%s:Peer id get success!\n",i_get_time());
            return tmpFile.owner;
        }
	}
	return -1;
}

int get_peer_info(struct msg *msg_recv, struct sockaddr *addr)
{
	//printf("a peer address response begin:\n");

    int nFileId = atoi(msg_recv->content);
	//printf("FileId = %d\n", nFileId);

    int nPeerId = get_peer_id(nFileId);
    char sPeerAddrprt[32] = {0};
    get_peer_addr(nPeerId, sPeerAddrprt);
	//printf("PeerAddr = %s\n", sPeerAddrprt);
    if (strlen(sPeerAddrprt) != 0)
    {
        msg_recv->flag = 6;
        msg_recv->id_to = msg_recv->id_from;
        msg_recv->id_from = 0;
        strcpy(msg_recv->content, sPeerAddrprt);
        strcat(msg_recv->content, ":");
        char name[256] = {0};
        get_file_path(nFileId, name);
        strcat(msg_recv->content, name);

        i_sendto(sockfd, (struct msg*)msg_recv, sizeof(struct msg), 0, addr, ADDR_LEN);
        //printf("msg_recv->content %s !\n", msg_recv->content);
        printf("%s:Id %d Peer %d Address %s get success!\n", i_get_time(), nFileId, nPeerId, sPeerAddrprt);
        return(EXIT_SUCCESS);
    }
    else
    {
        msg_recv->flag = -6;
        msg_recv->id_to = msg_recv->id_from;
        msg_recv->id_from = 0;
        i_sendto(sockfd, (struct msg*)msg_recv, sizeof(struct msg), 0, addr, ADDR_LEN);
        printf("%s:Id %d Peer %d Address %s get failed!\n", i_get_time(), nFileId, nPeerId, sPeerAddrprt);
        return(EXIT_FAILURE);
    }
}

int get_file_path(int fileId, char *fileName)
{
    struct file tmpFile;
    int posi;
    memset((struct file *)&tmpFile, 0, FL_LEN);

	for(posi = 1; posi <= fileNum; posi++)
	{
        i_lseek(file_list_fd, posi*FL_LEN, SEEK_SET);
        i_read(file_list_fd, &tmpFile, FL_LEN);

        if (fileId == tmpFile.id) {
            strcpy(fileName, tmpFile.name);
            return (EXIT_SUCCESS);
        }
	}
    return 0;
}

int msg_cntl()
{
	struct msg msg_recv;
	struct sockaddr addr_recv;
	printf("Begin listening...\n");
	int size = ADDR_LEN;
    struct sockaddr_in sin;
	for (;;)
	{
		bzero(&msg_recv, MSG_LEN);
		i_recvfrom(sockfd, &msg_recv, sizeof(struct msg), 0, &addr_recv, &size);
		printf("%s:message received...\n",i_get_time());

		switch (msg_recv.flag)
		{
			case 1 :
				create_user(&msg_recv, (struct sockaddr*)&addr_recv);
				break;
			case 2 :
				check_login(&msg_recv, (struct sockaddr*)&addr_recv);
				break;
			case 3:
				send_msg(&msg_recv,(struct sockaddr*)&addr_recv);
				break;
			case 4:
				register_file(&msg_recv, (struct sockaddr*)&addr_recv);
				break;
			case 5:
				search_file(&msg_recv, (struct sockaddr*)&addr_recv);
				break;
			case 6:
				get_peer_info(&msg_recv, (struct sockaddr*)&addr_recv);
				break;
			default :
				break;
		}
	}
	return(0);
}
/* end:message control*/

int listpeers()
{
    //i_listPeers(user_list_fd);
    return 0;
}
int listfiles()
{
    i_list_files(ffg);
    return 0;
    //i_listFiles(file_list_fd);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* start:exit_sys()*/
int exit_sys()
{
	close(sockfd);
    close(peer_cfg_fd);
	close(file_list_fd);
	fclose(ffg);
	fclose(fpg);
	printf("%s:exit system",i_get_time());
    //system("ps -ef | grep \"./server\" | awk -F\" \" \'{print $2}\' | xargs kill -9");
    kill(child, SIGABRT);
    kill(parent, SIGABRT);
	exit(0);
}
/* end:exit_sys()*/

/* start:menu*/
int menu()
{
	sleep(1);

	printf("----------help----menu---------\n");
	printf("\t h--help menu\n");
	printf("\t e--exit the system\n");
	printf("\t lp--list peers\n");
	printf("\t lf--list files\n");
	printf("\t sf--search file\n");
	printf("\t df--download file\n");
	printf("----------help_menu---------\n");

	char command[20];
	memset(command, 0, strlen(command));
	printf("input command>");
	scanf("%s", command);
	if(strcmp(command, "h") == 0)
	{
		menu();
	}
	else if(strcmp(command, "lf") == 0)
	{
		listfiles();
	}
	else if(strcmp(command, "lp") == 0)
	{
		listpeers();
	}
    else if(strcmp(command, "sf") == 0)
	{
		menu();
	}
    else if(strcmp(command, "e") == 0)
	{
		exit_sys();
	}
    printf(">>");

	return(0);
}
/* end:menu*/

int main()
{
	initialize();
	pid_t pid;
	switch (pid = fork())
	{
		case -1 :
			perror("fork error\n");
			exit(1);
			break;
		case 0 :
            child = getpid();
			msg_cntl();
			break;
		default :
            parent = getpid();
			menu();
			break;
	}
	return(0);
}
