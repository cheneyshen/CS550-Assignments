#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "hiredis.h"

typedef struct htpeer {
    char  *strIP; 
    int port;
} htpeer;

int main(int argc, char **argv) 
{
    unsigned int j;
    redisContext *c[16];
    redisReply *reply;
	htpeer server[16];
	server[0].strIP = "40.117.94.67";
	server[1].strIP = "40.117.92.166";
	server[2].strIP = "40.117.103.147";
	server[3].strIP = "104.41.145.114";
	server[4].strIP = "40.121.91.161";
	server[5].strIP = "40.117.91.244";
	server[6].strIP = "40.117.91.155";
	server[7].strIP = "40.121.91.83";
	server[8].strIP = "40.117.90.161";
	server[9].strIP = "40.121.95.5";
	server[10].strIP = "40.117.90.71";
	server[11].strIP = "40.121.91.190";
	server[12].strIP = "104.45.139.6";
	server[13].strIP = "40.117.88.60";
	server[14].strIP = "40.117.94.255";
	server[15].strIP = "40.114.7.144";
	int i;
	for (i = 0; i < 16; i++)
	{
		server[i].port = 6379;
	}
	struct timeval tval_writehash, tval_readhash, tval_deletehash, tval_done1, tval_done2;
	struct timeval max_write, max_read, max_delete;
    struct timeval min_write, min_read, min_delete;
	struct timeval tval_10K1, tval_10K2, tval_10K3;
	tval_10K1 = max_write = (struct timeval){0};
	tval_10K2 = max_read = (struct timeval){0};
	tval_10K3 = max_delete = (struct timeval){0};
    min_write = (struct timeval){10000};
    min_read = (struct timeval){10000};
    min_delete = (struct timeval){10000};
    char strkey[10];
    char strval[90];
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	for (i = 0; i <  16; i++)
	{
        c[i] = redisConnectWithTimeout(server[i].strIP, server[i].port, timeout);
        if (c[i] == NULL || c[i]->err) {
			if (c[i]) {
					printf("Connection error: %s\n", c[i]->errstr);
					redisFree(c[i]);
			} else {
					printf("Connection error: can't allocate redis context %d\n", i);
			}
			exit(1);
		}
    }

    /* PING server */
	for (i = 0; i < 16; i++) {
		reply = redisCommand(c[i],"PING");
		printf("PING: %s\n", reply->str);
    	freeReplyObject(reply);
	}
    for (j = 1000000000; j < 1000100000; j++) {
		int dest = rand() % 16;
        /* Set a key */
        snprintf(strkey, 10, "%u", j);
        gettimeofday(&tval_done1, NULL);
        reply = redisCommand(c[dest],"SET %s %s", strkey, "helloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworld");
        //printf("SET: %s\n", reply->str);
        gettimeofday(&tval_done2, NULL);
        timersub(&tval_done2, &tval_done1, &tval_writehash);
        timeradd(&tval_writehash, &tval_10K1, &tval_10K1);
		max_write = (timercmp(&max_write, &tval_writehash, <) ? tval_writehash : max_write);
        min_write = (timercmp(&min_write, &tval_writehash, >) ? tval_writehash : min_write);

        freeReplyObject(reply);
		/* Set a key using binary safe API
			reply = redisCommand(c[dest],"SET %b %b", "bar", (size_t) 3, "hello", (size_t) 5);
			printf("SET (binary API): %s\n", reply->str);
			freeReplyObject(reply);
		*/
        gettimeofday(&tval_done1, NULL);
        reply = redisCommand(c[dest],"GET %s", strkey);
        //printf("GET foo0000001: %s\n", reply->str);
        gettimeofday(&tval_done2, NULL);
        timersub(&tval_done2, &tval_done1, &tval_readhash);
        timeradd(&tval_readhash, &tval_10K2, &tval_10K2);
		max_read = (timercmp(&max_read, &tval_readhash, <) ? tval_readhash : max_read);
        min_read = (timercmp(&min_read, &tval_readhash, >) ? tval_readhash : min_read);
        freeReplyObject(reply);
        gettimeofday(&tval_done1, NULL);
        reply = redisCommand(c[dest],"DEL %s", strkey);
        //printf("DEL foo0000001: %s\n", reply->str);
        gettimeofday(&tval_done2, NULL);
        timersub(&tval_done2, &tval_done1, &tval_deletehash);
        timeradd(&tval_deletehash, &tval_10K3, &tval_10K3);
		max_delete = (timercmp(&max_delete, &tval_deletehash, <) ? tval_deletehash : max_delete);
        min_delete = (timercmp(&min_delete, &tval_deletehash, >) ? tval_deletehash : min_delete);
        freeReplyObject(reply);
    }
    printf("MAX write in 16 index servers: %ld.%06ld sec\n", (long int)max_write.tv_sec, (long int)max_write.tv_usec);
    printf("MIN write in 16 index servers: %ld.%06ld sec\n", (long int)min_write.tv_sec, (long int)min_write.tv_usec);
    printf("MAX read in 16 index servers: %ld.%06ld sec\n", (long int)max_read.tv_sec, (long int)max_read.tv_usec);
    printf("MIN read in 16 index servers: %ld.%06ld sec\n", (long int)min_read.tv_sec, (long int)min_read.tv_usec);
    printf("MAX delete in 16 index servers: %ld.%06ld sec\n", (long int)max_delete.tv_sec, (long int)max_delete.tv_usec);
    printf("MIN delete in 16 index servers: %ld.%06ld sec\n", (long int)min_delete.tv_sec, (long int)min_delete.tv_usec);
	printf("SET %d files in 16 index servers: %ld.%06ld sec\n", j, (long int)tval_10K1.tv_sec, (long int)tval_10K1.tv_usec);
	printf("GET %d files in 16 index servers: %ld.%06ld sec\n", j, (long int)tval_10K2.tv_sec, (long int)tval_10K2.tv_usec);
    printf("DEL %d files in 16 index servers: %ld.%06ld sec\n", j, (long int)tval_10K3.tv_sec, (long int)tval_10K3.tv_usec);

    /*
    reply = redisCommand(c,"INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);
     again ... 
    reply = redisCommand(c,"INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);
    */
    /* Create a list of numbers, from 0 to 9 */
    /*
    reply = redisCommand(c,"DEL mylist");
    freeReplyObject(reply);
    for (j = 0; j < 10; j++) {
    char buf[64];

    snprintf(buf,64,"%u",j);
    reply = redisCommand(c,"LPUSH mylist element-%s", buf);
    freeReplyObject(reply);
    }
    */
    /* Let's check what we have inside the list */
    /*
    reply = redisCommand(c,"LRANGE mylist 0 -1");
    if (reply->type == REDIS_REPLY_ARRAY) {
    for (j = 0; j < reply->elements; j++) {
        printf("%u) %s\n", j, reply->element[j]->str);
    }
    }
    freeReplyObject(reply);
    */
    /* Disconnects and frees the context */
	for (i = 0; i < 16; i++) {
    	redisFree(c[i]);
	}

    return 0;
}