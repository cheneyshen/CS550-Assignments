

void * put_file(void *arg)
{
    proclinfo *info = arg;
    char str_opr[10] = {0};
    char str_name[256] = {0};
    char str_result[1024] = {0};
    if (0 == strncasecmp(info->buffer, "get", 3) || 0 == strncasecmp(info->buffer, "g", 1))
    {
        sscanf(info->buffer, "%[^ ] %[^ ]", str_opr, str_name);

        //if (-1 == write(info->socket, str_result, strlen(str_result)))
        //    FUNW("write");
        //memset(str_result, 0, sizeof(str_result));

        printf("downloading...\n");
        fd=open(filename, O_RDWR|O_CREAT, RWRWRW);
        if (-1 == fd)
        {
            perror("Cannot open output file\n"); exit(1);
        }
        while (filesize > 0) {
            n=read(sockfd,filebuf,BUFSIZE);
            write(fd,filebuf,n);
            filesize-=n;
        }
        printf("done!");
        close(fd);
    }

    FILE *fp = NULL;
    fp = fopen(str_key, "rb");
    if (fp ==  NULL)
    {
        printf("%s:no file can't be downoaded\n", i_get_time());
        strcpy(str_result, "Not Found");
        if (-1 == write(info->socket, str_result, strlen(str_result)))
            FUNW("write");
        return ((void *)NULL);
    }
    else
    {
        tFile.filesize = filesize(fp);
        printf("%s:file size: %d\n", i_get_time(), tFile.filesize);
        memcpy(file_send_msg.content, (unsigned char *)&tFile, FL_LEN);
        i_sendto(sockfd, (struct msg*)&file_send_msg, MSG_LEN, 0, (struct sockaddr*)&to_addr, ADDR_LEN);
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