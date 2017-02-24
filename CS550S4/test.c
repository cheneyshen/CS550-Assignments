void * thread_backup(void *arg)
{
        proclinfo *cpfile = arg;
        // This is Backup server, run file replication service
        htpeer provider;
        char s_ip[16] = {0};
        char s_port[5] = {0};
        char address[25] = {0};
        char fileinfo[20] = {0};
        char filepath[256] = {0};
        char str_result[BUFSIZE] = {0};
        char *ptr, r = '/';
        int filesize;
        int wn = 0;
        FILE *fp;
        struct timeval timeout = {5,0};
        memset(&provider, 0, sizeof(htpeer));

        //buf = "regist  /home/sf/aaa.txt 127.0.0.1:8888"
        sscanf(cpfile->buffer, "%[^ ] %[^ ] %s", fileinfo, filepath, address);
        sscanf(address, "%[^:]: %[^ ]", s_ip, s_port);
        inet_aton(s_ip, &(provider.addr.sin_addr));
         // Do not forget initialize sin_family, will result in read error
        provider.addr.sin_family = AF_INET;
        provider.addr.sin_port = htons(atoi(s_port));
        if (-1 == (provider.fd = socket(AF_INET, SOCK_STREAM, 0)))
            FUNW("socket");
        if (-1 == connect(provider.fd, (struct sockaddr *)(&(provider.addr)), sizeof(struct sockaddr)))
            FUNW("connect");
        printf("Connected to connectfd:%d host:%s:%d\n", provider.fd, inet_ntoa(provider.addr.sin_addr), htons(provider.addr.sin_port));

        strcpy(str_result, "obtain /home/cheney/happyball.jpg");
        //strcat(str_result, filepath);
        str_result[strlen(str_result)] = '\0';
        printf("str_result:%s", str_result);
        if (-1 == write(provider.fd, str_result, strlen(str_result)))
            FUNW("write");
        if (-1 == recv(provider.fd, fileinfo, sizeof(fileinfo), 0))
            FUNW("recv");
        printf("filesize:%s\n", fileinfo);
        if((filesize = atoi(fileinfo)) == 0) {
            printf("get file size error!");
        }
        //Save to ./Download/ folder
        sprintf(str_result, "./Backup");
        if (-1 == access(str_result, X_OK))
        {
            int status = mkdir(filepath, S_IRWXU | S_IRWXG | S_IRWXO); /*777*/
            (!status) ? (printf("Directory created %s\n", filepath)) : (printf("Unable to create directory %s\n", filepath));
        }
        ptr = strrchr(filepath, r);
        strcat(str_result, ptr);

        if( ( fp = fopen(str_result,"wb") ) == NULL )
            FUNW("fopen");
        printf("Downloading %s...\n", filepath);
        while( 1 )
        {
            wn = recv(provider.fd, str_result, BUFSIZE, 0);
            fwrite(str_result, 1, wn, fp);
            printf("wn=%d\n", wn);
            filesize -= wn;
            if (filesize  == 0 )    // failed connection processing
                break;
            else if( 0 > wn || (wn == 0 && filesize > 0) ) {
                printf("Lost connection to socket fd: %d provider\n", provider.fd);
                break;
            }
        }
        if(-1 == fclose(fp))
            FUNW("fclose");
        printf("Done!\n");
        strcpy(str_result, "quit peer\n");
        if (provider.fd > 0 && -1 ==write(provider.fd, str_result, strlen(str_result)))
            FUNW("write");
        close(provider.fd);
        return ((void *)NULL);

}

/*

Copyright 2015 Jonathan Watmough
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
	http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

//#ifdef HASHTEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jwHash.h"
#include <sys/time.h>

#ifdef HASHTHREADED
#include <pthread.h>
#include <semaphore.h>
#endif

#ifdef _WIN32
static inline void timersub(
	const struct timeval *t1,
	const struct timeval *t2,
	struct timeval *res)
{
	res->tv_sec = t1->tv_sec - t2->tv_sec;
	if( t1->tv_usec - t2->tv_usec < 0 ) {
		res->tv_sec--;
		res->tv_usec = t1->tv_usec - t2->tv_usec + 1000 * 1000;
	} else {
		res->tv_usec = t1->tv_usec - t2->tv_usec;
	}
}
#endif

int basic_test();
int thread_test();

int main(int argc, char *argv[])
{
	if( 0==basic_test() ) {
		printf("basic_test:\tPassed\n");
	}
#ifdef HASHTHREADED
	if( 0==thread_test() ) {
		printf("thread_test:\tPassed\n");
	}
#endif
	return 0;
}

int basic_test()
{
	// create
	//csHashTable * table = create_hash(10);
	csHashTable * table = create_hash(1);
	if(!table) {
		// fail
		return 1;
	}

	// add a few values
	char * str1 = "string 1";
	char * str2 = "string 2";
	char * str3 = "string 3";
	char * str4 = "string 4";
	char * str5 = "string 5";
	add_str_by_int(table,0,str1);
	add_str_by_int(table,1,str2);
	add_str_by_int(table,2,str3);
	add_str_by_int(table,3,str4);
	add_str_by_int(table,4,str5);

	char * sstr1; get_str_by_int(table,0,&sstr1);
	char * sstr2; get_str_by_int(table,1,&sstr2);
	char * sstr3; get_str_by_int(table,2,&sstr3);
	char * sstr4; get_str_by_int(table,3,&sstr4);
	char * sstr5; get_str_by_int(table,4,&sstr5);
	printf("got strings: \n1->%s\n2->%s\n3->%s\n4->%s\n5->%s\n",sstr1,sstr2,sstr3,sstr4,sstr5);

	del_by_int(table, 0);
	del_by_int(table, 1);
	del_by_int(table, 2);
	del_by_int(table, 3);
	del_by_int(table, 4);

	// Test hashing by string
	char * strv1 = "Jonathan";
	char * strv2 = "Zevi";
	char * strv3 = "Jude";
	char * strv4 = "Voldemort";

	add_str_by_str(table,"oldest",strv1);
	add_str_by_str(table,"2ndoldest",strv2);
	add_str_by_str(table,"3rdoldest",strv3);
	add_str_by_str(table,"4tholdest",strv4);

	char * sstrv1; get_str_by_str(table,"oldest",&sstrv1);
	char * sstrv2; get_str_by_str(table,"2ndoldest",&sstrv2);
	char * sstrv3; get_str_by_str(table,"3rdoldest",&sstrv3);
	char * sstrv4; get_str_by_str(table,"4tholdest",&sstrv4);
	printf("got strings:\noldest->%s \n2ndoldest->%s \n3rdoldest->%s \n4tholdest->%s\n",
		sstrv1,sstrv2,sstrv3,sstrv4);
	return 0;
}

//#ifdef HASHTHREADED

#define NUMTHREADS 6
#define HASHCOUNT 100000

typedef struct threadinfo {csHashTable *table; int start;} threadinfo;
void * thread_func(void *arg)
{
	threadinfo *info = arg;
	char buffer[512];
	int i = info->start;
	csHashTable *table = info->table;
	free(info);
	for(;i<HASHCOUNT;i+=NUMTHREADS) {
		sprintf(buffer,"%d",i);
		add_int_by_str(table,buffer,i);
	}
}


int thread_test()
{
	// create
	csHashTable * table = create_hash(HASHCOUNT>>2);

	// hash a million strings into various sizes of table
	struct timeval tval_before, tval_done1, tval_done2, tval_writehash, tval_readhash;
	gettimeofday(&tval_before, NULL);
	int t;
	pthread_t * threads[NUMTHREADS];
	for(t=0;t<NUMTHREADS;++t) {
		pthread_t * pth = malloc(sizeof(pthread_t));
		threads[t] = pth;
		threadinfo *info = (threadinfo*)malloc(sizeof(threadinfo));
		info->table = table; info->start = t;
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
		get_int_by_str(table,buffer,&j);
		if(i!=j) {
			printf("Error: %d != %d\n",i,j);
			error = 1;
		}
	}
	if(!error) {
		printf("No errors.\n");
	}
	gettimeofday(&tval_done2, NULL);
	timersub(&tval_done1, &tval_before, &tval_writehash);
	timersub(&tval_done2, &tval_done1, &tval_readhash);
	printf("\n%d threads.\n",NUMTHREADS);
	printf("Store %d ints by string: %ld.%06ld sec, read %d ints: %ld.%06ld sec\n",HASHCOUNT,
		(long int)tval_writehash.tv_sec, (long int)tval_writehash.tv_usec,HASHCOUNT,
		(long int)tval_readhash.tv_sec, (long int)tval_readhash.tv_usec);

	return 0;
}
    char str_opr[10] = {0};
    char str_key[256] = {0};
    int result;
    char str_result[1000] = {0};

    int t;
	pthread_t * threads[NUMTHREADS];
    gettimeofday(&tval_before, NULL);
	for(t=0;t<NUMTHREADS;++t) {
		pthread_t * pth = malloc(sizeof(pthread_t));
		threads[t] = pth;
		threadinfo *info = (threadinfo*)malloc(sizeof(threadinfo));
		info->table = table; info->start = t;
		pthread_create(pth,NULL,thread_put,info);
	}
	for(t=0;t<NUMTHREADS;++t) {
		pthread_join(*threads[t], NULL);
	}
	gettimeofday(&tval_done1, NULL);
    for(t=0;t<NUMTHREADS;++t) {
		pthread_t * pth = malloc(sizeof(pthread_t));
		threads[t] = pth;
		threadinfo *info = (threadinfo*)malloc(sizeof(threadinfo));
		info->table = table; info->start = t;
		pthread_create(pth,NULL,thread_get,info);
	}
	for(t=0;t<NUMTHREADS;++t) {
		pthread_join(*threads[t], NULL);
	}
	gettimeofday(&tval_done2, NULL);
    for(t=0;t<NUMTHREADS;++t) {
		pthread_t * pth = malloc(sizeof(pthread_t));
		threads[t] = pth;
		threadinfo *info = (threadinfo*)malloc(sizeof(threadinfo));
		info->table = table; info->start = t;
		pthread_create(pth,NULL,thread_del,info);
	}
	for(t=0;t<NUMTHREADS;++t) {
		pthread_join(*threads[t], NULL);
	}
	gettimeofday(&tval_done3, NULL);
	timersub(&tval_done1, &tval_before, &tval_writehash);
	timersub(&tval_done2, &tval_done1, &tval_readhash);
	timersub(&tval_done3, &tval_done2, &tval_deletehash);
    printf("Store 100000 strings by int in %d threads for single server: %ld.%06ld sec, read: %ld.%06ld sec, delete: %ld.%06ld sec\n",
        NUMTHREADS,
		(long int)tval_writehash.tv_sec, (long int)tval_writehash.tv_usec,
		(long int)tval_readhash.tv_sec, (long int)tval_readhash.tv_usec,
		(long int)tval_deletehash.tv_sec, (long int)tval_deletehash.tv_usec);

		pthread_t tids;
		pthread_create(&tids, NULL, server_talk, (void *)argv);
		pthread_join(tids, NULL);


    int t;
	pthread_t * threads[NUMTHREADS];
    gettimeofday(&tval_before, NULL);
	for(t=0;t<NUMTHREADS;++t) {
		pthread_t * pth = malloc(sizeof(pthread_t));
		threads[t] = pth;
		threadinfo *info = (threadinfo*)malloc(sizeof(threadinfo));
		info->table = pcinfo->table; info->start = t;
		pthread_create(pth,NULL,thread_put,info);
	}
	for(t=0;t<NUMTHREADS;++t) {
		pthread_join(*threads[t], NULL);
	}
	gettimeofday(&tval_done1, NULL);
    for(t=0;t<NUMTHREADS;++t) {
		pthread_t * pth = malloc(sizeof(pthread_t));
		threads[t] = pth;
		threadinfo *info = (threadinfo*)malloc(sizeof(threadinfo));
		info->table = pcinfo->table; info->start = t;
		pthread_create(pth,NULL,thread_get,info);
	}
	for(t=0;t<NUMTHREADS;++t) {
		pthread_join(*threads[t], NULL);
	}
	gettimeofday(&tval_done2, NULL);
    for(t=0;t<NUMTHREADS;++t) {
		pthread_t * pth = malloc(sizeof(pthread_t));
		threads[t] = pth;
		threadinfo *info = (threadinfo*)malloc(sizeof(threadinfo));
		info->table = pcinfo->table; info->start = t;
		pthread_create(pth,NULL,thread_del,info);
	}
	for(t=0;t<NUMTHREADS;++t) {
		pthread_join(*threads[t], NULL);
	}
	gettimeofday(&tval_done3, NULL);
	timersub(&tval_done1, &tval_before, &tval_writehash);
	timersub(&tval_done2, &tval_done1, &tval_readhash);
	timersub(&tval_done3, &tval_done2, &tval_deletehash);
    printf("Store 100000 strings by int in %d threads for single server: %ld.%06ld sec, read: %ld.%06ld sec, delete: %ld.%06ld sec\n",
        NUMTHREADS,
		(long int)tval_writehash.tv_sec, (long int)tval_writehash.tv_usec,
		(long int)tval_readhash.tv_sec, (long int)tval_readhash.tv_usec,
		(long int)tval_deletehash.tv_sec, (long int)tval_deletehash.tv_usec);



void * thread_put(void *arg)
{
	threadinfo *info = arg;
	char buffer[512];
	int i = info->start;
	csHashTable *table = info->table;
	free(info);
	for(;i<HASHCOUNT;i+=NUMTHREADS) {
		sprintf(buffer,"%d",i);
		add_int_by_str(table,buffer,i);
	}
}

void * thread_get(void *arg)
{
	threadinfo *info = arg;
	int j;
	int error = 0;
	int i = info->start;
	char buffer[512];
	csHashTable *table = info->table;
	free(info);
	for( ;i<HASHCOUNT;i+=NUMTHREADS) {
		sprintf(buffer,"%d",i);
		get_int_by_str(table,buffer,&j);
		if (i!=j) {
			printf("Error: %d != %d\n",i,j);
			error = 1;
		}
	}
	//if (!error) {
		//printf("No errors.\n");
	//}
}

void * thread_del(void *arg)
{
	threadinfo *info = arg;
	int j;
	int i = info->start;
	char buffer[512];
	csHashTable *table = info->table;
	free(info);
	for( ;i<HASHCOUNT;i+=NUMTHREADS) {
		sprintf(buffer,"%d",i);
		del_by_str(table, buffer);
	}
}



void * thread_put(void *arg)
{
	int *info = arg;
	char buf[BUFSIZE] = {0};
    char str_key[256] = {0};
	int i = *info;int nbytes;
    char * str_put = "put ";
	for(i =HASHCOUNT+ i  ;i< 2*HASHCOUNT;i+=NUMTHREADS) {
        strcpy(buf, str_put);
        sprintf(str_key, "%d", i);
        strncat(buf, str_key, strlen(str_key));
        strcat(buf, " ");
        strcat(buf, "string");
        strncat(buf, str_key, strlen(str_key));
        buf[strlen(buf)] = '\0';

        size_t hash = hashInt(atoi(str_key)) % 2;
        #ifdef HASHTHREADED
        // lock this bucket against changes
        // Refer to Multithreaded simple data type access and atomic variables
        while (__sync_lock_test_and_set(&server[hash].lock, 1)) {
            // Do nothing. This GCC builtin instruction
            // ensures memory barrier.
          }
        #endif
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
        unlock:
        #ifdef HASHTHREADED
            __sync_synchronize(); // memory barrier
            server[hash].lock = 0;
        #endif
        bzero(buf, BUFSIZE);
        buf[0] = '\0';
	}
}

void * thread_get(void *arg)
{
	int *info = arg;
	char buf[BUFSIZE] = {0};
    char str_key[256] = {0};
	int i = *info;int nbytes;
    char * str_get = "get ";

	for(i =HASHCOUNT+ i  ;i< 2*HASHCOUNT;i+=NUMTHREADS) {
        strcpy(buf, str_get);
        sprintf(str_key, "%d", i);
        strncat(buf, str_key, strlen(str_key));
        buf[strlen(buf)] = '\0';

        size_t hash = hashInt(atoi(str_key)) % 2;
        #ifdef HASHTHREADED
        // lock this bucket against changes
        // Refer to Multithreaded simple data type access and atomic variables
        while (__sync_lock_test_and_set(&server[hash].lock, 1)) {
            // Do nothing. This GCC builtin instruction
            // ensures memory barrier.
          }
        #endif
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
        unlock:
        #ifdef HASHTHREADED
            __sync_synchronize(); // memory barrier
            server[hash].lock = 0;
        #endif
        bzero(buf, BUFSIZE);
        buf[0] = '\0';
	}
}

void * thread_del(void *arg)
{
	int *info = arg;
	char buf[BUFSIZE] = {0};
    char str_key[256] = {0};
	int i = *info;int nbytes;
    char * str_del = "del ";

	for(i =HASHCOUNT+ i  ;i< 2*HASHCOUNT;i+=NUMTHREADS) {
        strcpy(buf, str_del);
        sprintf(str_key, "%d", i);
        strncat(buf, str_key, strlen(str_key));
        buf[strlen(buf)] = '\0';

        size_t hash = hashInt(atoi(str_key)) % 2;
        #ifdef HASHTHREADED
        // lock this bucket against changes
        // Refer to Multithreaded simple data type access and atomic variables
        while (__sync_lock_test_and_set(&server[hash].lock, 1)) {
            // Do nothing. This GCC builtin instruction
            // ensures memory barrier.
          }
        #endif
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
        unlock:
        #ifdef HASHTHREADED
            __sync_synchronize(); // memory barrier
            server[hash].lock = 0;
        #endif
        bzero(buf, BUFSIZE);
        buf[0] = '\0';
	}
}



if (0 == strncasecmp(buf, "put", 3))
				{

                    char * str_put = "put ";
                    int key, result;
                    gettimeofday(&tval_before, NULL);
                    int t;
                    for(t=0;t<NUMTHREADS;++t) {
                        pthread_t * pth = malloc(sizeof(pthread_t));
                        threads[t] = pth;
                        pthread_create(pth,NULL,thread_put,&t);
                    }
                    for(t=0;t<NUMTHREADS;++t) {
                        pthread_join(*threads[t], NULL);
                    }
                    /*
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

                    }*/
                    printf("done1\n");
                    gettimeofday(&tval_done1, NULL);
				}
				else if (0 == strncasecmp(buf, "get", 3))
				{
                    char * str_get = "get ";
                    int key, result;
                    //sscanf(buf, "%[^ ] %[^ ]", str_cmd, str_key);
                    //printf("%s\n", str_key);
                    //size_t hash = hashInt(atoi(str_key)) % serv_cnt;
                    //if (-1 == (nbytes = send(server[hash].fd, buf, strlen(buf), 0)))
                    //   FUNW("write");

                    gettimeofday(&tval_done2, NULL);
                    int t;
                    for(t=0;t<NUMTHREADS;++t) {
                        pthread_t * pth = malloc(sizeof(pthread_t));
                        threads[t] = pth;
                        pthread_create(pth,NULL,thread_get, &t);
                    }
                    for(t=0;t<NUMTHREADS;++t) {
                        pthread_join(*threads[t], NULL);
                    }
                    /*
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
                    */
                    printf("done2\n");
                    gettimeofday(&tval_done3, NULL);
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
                    int t;
                    for(t=0;t<NUMTHREADS;++t) {
                        pthread_t * pth = malloc(sizeof(pthread_t));
                        threads[t] = pth;
                        pthread_create(pth,NULL,thread_del, &t);
                    }
                    for(t=0;t<NUMTHREADS;++t) {
                        pthread_join(*threads[t], NULL);
                    }
                    /*
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
                    */
                    printf("done3\n");
                    gettimeofday(&tval_done5, NULL);
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
//#endif
//#endif





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



else if (0 == strncasecmp(buf, "conn", 4))
{
    //printf("IP:Port:%s,%s,%s\n", s_command, s_IP, s_port);
    //getchar();
    pthread_t tidc;
    pthread_create(&tidc, NULL, peer_talk, (void *)argu);
    pthread_join(tidc, NULL);
}



                    //pthread_t *pth = malloc(sizeof(pthread_t));
                    //proclinfo *info = (proclinfo*)malloc(sizeof(proclinfo));
                    //info->buffer = buf;
                    //info->socket = provider.fd;
                    //get_file((void *)info);
                    //printf("get file ok!");
                    //pthread_create(pth, NULL, get_file, info);
                    //FD_CLR(provider.fd, &ordfs); provider.fd = -1;
                    //proclinfo *info = arg;
                    char filebuf[BUFSIZE] = "\0";  // used to read file
                    char fileinfo[20] = {0};
                    char filename[256] = {0};
                    char *ptr, r = '/';
                    int filesize;
                    int fd;
                    int rn = 0;
                    FILE *fp;
                    msg commsg;

                    sscanf(buf, "%[^ ] %[^ ]", fileinfo, filename);
                    //info->buffer[(int)strlen(info->buffer)] = '\0';
                    if (-1 == write(provider.fd, buf, BUFSIZE))
                        FUNW("write");
                    /*
                    bzero(buf, BUFSIZE);
                    strcpy(buf, "***OK***");
                    if (-1 == write(provider.fd, buf, BUFSIZE))
                        FUNW("write");
                    if (-1 == read(provider.fd, fileinfo, sizeof(fileinfo)))
                        FUNW("read");
                    printf("%s\n", fileinfo);
                    if((filesize = atoi(fileinfo)) == 0) {
                        printf("get file error!");
                    }
                    else
                    {
                    */
                        printf("downloading...\n");
                        //Save to ./Download/ folder
                        char filepath[256];
                        sprintf(filepath, "./Download");
                        if (-1 == access(filepath, X_OK))
                        {
                            int status = mkdir(filepath, S_IRWXU | S_IRWXG | S_IRWXO); /*777*/
                            (!status) ? (printf("Directory created %s\n", filepath)) : (printf("Unable to create directory %s\n", filepath));
                        }
                        ptr = strrchr(filename, r);
                        strcat(filepath, ptr);
                        printf("%s\n", filepath);

                        if( ( fp = fopen(filepath,"wb") ) == NULL )
                        {
                                perror("File");pthread_exit(NULL);
                        }
                        while( 1 )
                        {
                            rn = recv(provider.fd, filebuf, BUFSIZE, 0);
                            printf("rn=%d\n", rn);
                            if(rn == 0)
                                break;
                            fwrite(filebuf, 1, rn, fp);
                        }
/*
                        while( 1 )
                        {
                            bzero(filebuf, sizeof(filebuf));

                            if(filesize <= BUFSIZE) {
                                rn = read(provider.fd, filebuf, filesize);
                                printf("rn=%d\n", rn);
                                fwrite(filebuf, 1, filesize, fp);
                                break;
                            }
                            else {
                                rn = read(provider.fd, filebuf, BUFSIZE);
                                printf("rn=%d\n", rn);
                                fwrite(filebuf, 1, BUFSIZE, fp);
                            }
                            filesize -=BUFSIZE;
                            printf("filesize=%d\n", filesize);
                        }

                        while( 1 )
                        {
                            bzero(&commsg, sizeof(commsg));
                            recv(provider.fd, (msg *)&commsg, sizeof(commsg), 0);
                            printf("len=%d\n", commsg.len);

                            if(commsg.len < BUFSIZE) {
                                fwrite(commsg.content, 1, commsg.len, fp);
                                break;
                            }
                            else {
                                fwrite(commsg.content, 1, BUFSIZE, fp);
                            }
                            filesize +=commsg.len;
                            printf("filesize=%d\n", filesize);
                        }
                        */
                        if(-1 == fclose(fp))
                            FUNW("fclose");
                        printf("done!\n");
#ifdef DRAFT
                else if (0 == strncasecmp(buf, "obtain", 6) || 0 == strncasecmp(buf, "o", 1))
                {
                    // Downloading File
                    buf[nbytes - 1] = '\0'; // read nbytes bytes
                    char fileinfo[20] = {0};
                    char filepath[256] = {0};
                    char address[25] = {0};
                    char s_ip[16] = {0};
                    char s_port[6] = {0};
                    htpeer provider;
                    memset(&provider, 0, sizeof(htpeer));

                    //buf = "regist  /home/sf/aaa.txt 127.0.0.1:8888"
                    sscanf(buf, "%[^ ] %[^ ] %s", fileinfo, filepath, address);
                    sscanf(address, "%[^:]: %[^ ]", s_ip, s_port);
                    inet_aton(s_ip, &(provider.addr.sin_addr));
                     // Do not forget initialize sin_family, will result in read error
                    provider.addr.sin_family = AF_INET;
                    provider.addr.sin_port = htons(atoi(s_port));
                    if (-1 == (provider.fd = socket(AF_INET, SOCK_STREAM, 0)))
                        FUNW("socket");
                    if (-1 == connect(provider.fd, (struct sockaddr *)(&(provider.addr)), sizeof(struct sockaddr)))
                        FUNW("connect");
                    printf("Connected to connectfd:%d host:%s:%d\n", provider.fd, inet_ntoa(provider.addr.sin_addr), htons(provider.addr.sin_port));
                    FD_SET(provider.fd, &allfs);
                    //connfd is the best file descriptor which in the rdfs(read file set)
                    maxfd = maxfd > provider.fd ? maxfd : provider.fd;
                    pthread_t *pth = malloc(sizeof(pthread_t));
                    proclinfo *info = (proclinfo*)malloc(sizeof(proclinfo));
                    info->buffer = filepath;
                    info->socket = provider.fd;
                    pthread_create(pth, NULL, thread_backup, info);
                }
#endif // DRAFT
