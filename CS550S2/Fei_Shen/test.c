//Code snippet

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
