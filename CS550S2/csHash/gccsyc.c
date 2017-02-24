/*************************************************************************
	> File Name: gccsyc.c
	> Author: 
	> Mail: 
	> Created Time: Sat 03 Oct 2015 08:29:20 PM CDT
 ************************************************************************/

#include<stdio.h>
#include <pthread.h>
#include <stdlib.h>

static int count = 0;
pthread_mutex_t lock;

void *test_func(void *arg)
{
	int i=0;
    int mlock = *(int *)arg;
	for(i=0;i<20000;++i){
        //__sync_lock_test_and_set(&lock, 1);
		__sync_fetch_and_add(&count,1);
        //pthread_mutex_lock(&lock);
        //count++;	
        //__sync_lock_release(&lock);
        //pthread_mutex_unlock(&lock);
	}
	return NULL;

}

int main(int argc, const char *argv[])
{
	pthread_t id[20];
	int i = 0;

	for(i=0;i<20;++i){
		pthread_create(&id[i],NULL,test_func,&i);
	
	}

	for(i=0;i<20;++i){
		pthread_join(id[i],NULL);
	
	}

	printf("%d\n",count);
	return 0;

}
