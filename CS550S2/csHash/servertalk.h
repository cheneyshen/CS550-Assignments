#ifndef SERVERTALK_H_INCLUDED
#define SERVERTALK_H_INCLUDED


//void process_cli(clientinfo *client, int socketfd, char* recvbuf, jwHashTable *hashtable)
void * process_cli(void *arg)
{
    procli_info *info = arg;
    char str_opr[10] = {0};
    char str_key[256] = {0};
    int result;
    char str_result[1000] = {0};
    gettimeofday(&tval_before, NULL);
    gettimeofday(&tval_done1, NULL);
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
    printf("Store 100000 strings by int in %d threads for single server: %ld.%06ld sec, read: %ld.%06ld sec, delete: %ld.%06ld sec\n",
        NUMTHREADS,
		(long int)tval_writehash.tv_sec, (long int)tval_writehash.tv_usec,
		(long int)tval_readhash.tv_sec, (long int)tval_readhash.tv_usec,
		(long int)tval_deletehash.tv_sec, (long int)tval_deletehash.tv_usec);

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
                    char s_IP[16] = {0};
                    char s_port[6] = {0};
                    char s_command[5] = {0};
                    sscanf(buf, "%[^ ] %[^ ] %[^ ]", s_command, s_IP, s_port);
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
                    //printf(">%s\n", buf);
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

#endif // SERVERTALK_H_INCLUDED
