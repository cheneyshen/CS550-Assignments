
				else if (0 == strncasecmp(buf, "put", 3))
				{
                    char * str_put = "put ";
                    int key, result;
                    gettimeofday(&tval_before, NULL);
                    for (key = 100000; key <= 200000; key++)
                    {
                        //buf = "put 100000 string100000"
                        strcpy(buf, str_put);
                        sprintf(str_key, "%d", key);
                        strncat(buf, str_key, strlen(str_key));
                        strcat(buf, " ");
                        strcat(buf, "string");
                        strncat(buf, str_key, strlen(str_key));
                        buf[strlen(buf)] = '\0';
                        //char str_value[718] ={0};
                        //sscanf(buf, "%[^ ] %[^ ] %s", str_cmd, str_key, str_value);
                        //send to server[hash]
                        size_t hash = hashInt(atoi(str_key)) % serv_cnt;

                        //if (-1 == (nbytes = write(server[hash].fd, buf, strlen(buf))))
                        if (-1 == (nbytes = send(server[hash].fd, buf, strlen(buf), 0)))
                            FUNW("write");
                        bzero(buf, BUFSIZE);
                        buf[0] = '\0';

                        if (-1 == (nbytes = recv(server[hash].fd, buf, BUFSIZE, 0)))
                            FUNW("read2");
                        if (0 == strncasecmp(buf, "quit", 4) || 0 == nbytes)
                        {
                            printf("he quit!\npress any key to quit!\n");
                            getchar();
                            break;
                        }
                        //fprintf(stdout, "%s.%u:%s\n", inet_ntoa(server[n].addr.sin_addr), htons(server[n].addr.sin_port), buf);
                        //fprintf(stdout, "%s\n", translate_result(buf));
                        bzero(buf, BUFSIZE);
                        buf[0] = '\0';
                        //if(--ready_to <= 0) break;  // mo more readable descriptors

                    }
                    gettimeofday(&tval_done1, NULL);
                    printf("done!\n")
				}
				else if(0 == strncasecmp(buf, "get", 3))
				{
                    char * str_get = "get ";
                    int key, result;
                    //sscanf(buf, "%[^ ] %[^ ]", str_cmd, str_key);
                    //printf("%s\n", str_key);
                    //size_t hash = hashInt(atoi(str_key)) % serv_cnt;
                    //if (-1 == (nbytes = send(server[hash].fd, buf, strlen(buf), 0)))
                    //   FUNW("write");

                    gettimeofday(&tval_done2, NULL);
                    for (key = 100000; key <= 200000; key++)
                    {
                        strcpy(buf, str_get);
                        sprintf(str_key, "%d", key);
                        strncat(buf, str_key, strlen(str_key));
                        buf[strlen(buf)] = '\0';
                        size_t hash = hashInt(atoi(str_key)) % serv_cnt;
                        if (-1 == (nbytes = send(server[hash].fd, buf, strlen(buf), 0)))
                            FUNW("write");
                        bzero(buf, BUFSIZE);
                        buf[0] = '\0';

                        if (-1 == (nbytes = recv(server[hash].fd, buf, BUFSIZE, 0)))
                            FUNW("read2");
                        if (0 == strncasecmp(buf, "quit", 4) || 0 == nbytes)
                        {
                            printf("he quit!\npress any key to quit!\n");
                            getchar();
                            break;
                        }
                        //fprintf(stdout, "%s.%u:%s\n", inet_ntoa(server[n].addr.sin_addr), htons(server[n].addr.sin_port), buf);
                        //fprintf(stdout, "%s\n", translate_result(buf));
                        bzero(buf, BUFSIZE);
                        buf[0] = '\0';
                    }
                    gettimeofday(&tval_done3, NULL);
                    printf("done!\n")
				}
				else if(0 == strncasecmp(buf, "del", 3))
				{
                    //sscanf(buf, "%[^ ] %[^ ]", str_cmd, str_key);
                    //printf("%s\n", str_key);
                    //size_t hash = hashInt(atoi(str_key)) % serv_cnt;
                    //if (-1 == (nbytes = send(server[hash].fd, buf, strlen(buf), 0)))
                       // FUNW("write");

                    char * str_del = "del ";
                    int key, result;
                    gettimeofday(&tval_done4, NULL);
                    for (key = 100000; key <= 200000; key++)
                    {
                        strcpy(buf, str_del);
                        sprintf(str_key, "%d", key);
                        strncat(buf, str_key, strlen(str_key));
                        buf[strlen(buf)] = '\0';
                        size_t hash = hashInt(atoi(str_key)) % serv_cnt;
                        if (-1 == (nbytes = send(server[hash].fd, buf, strlen(buf), 0)))
                            FUNW("write");
                        bzero(buf, BUFSIZE);
                        buf[0] = '\0';

                        if (-1 == (nbytes = recv(server[hash].fd, buf, BUFSIZE, 0)))
                            FUNW("read2");
                        if (0 == strncasecmp(buf, "quit", 4) || 0 == nbytes)
                        {
                            printf("he quit!\npress any key to quit!\n");
                            getchar();
                            break;
                        }
                        //fprintf(stdout, "%s.%u:%s\n", inet_ntoa(server[n].addr.sin_addr), htons(server[n].addr.sin_port), buf);
                        //fprintf(stdout, "%s\n", translate_result(buf));
                        bzero(buf, BUFSIZE);
                        buf[0] = '\0';
                    }
                    gettimeofday(&tval_done5, NULL);
                    printf("done!\n")
				}
				else if (0 == strncasecmp(buf, "time", 4))
				{
                    timersub(&tval_done1, &tval_before, &tval_writehash);
                    timersub(&tval_done3, &tval_done2, &tval_readhash);
                    timersub(&tval_done5, &tval_done4, &tval_deletehash);
                    printf("Store 100000 strings by int: %ld.%06ld sec, read: %ld.%06ld sec, delete: %ld.%06ld sec\n",
                    (long int)tval_writehash.tv_sec, (long int)tval_writehash.tv_usec,
                    (long int)tval_readhash.tv_sec, (long int)tval_readhash.tv_usec,
                    (long int)tval_deletehash.tv_sec, (long int)tval_deletehash.tv_usec);
				}
