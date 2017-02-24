/*
 * * =====================================================================================
 * *
 * *   Filename:  atomic.c
 * *
 * *Description:  gcc-4.1.1: Built-in functions for atomic memory access
 * *  http://gcc.gnu.org/onlinedocs/gcc-4.1.1/gcc/Atomic-Builtins.html
 *
 * compile:
 *  gcc atomic.c -lpthead
 *  run:
 *  time ./a.out 
 *
 *  modify Macro VER to choose the version
 *  1. no lock
 *  2. pthread_mutex
 *  3. atomic
 *  ref:
 *  http://gcc.gnu.org/onlinedocs/gcc-4.1.1/gcc/Atomic-Builtins.html
 *  http://www.alexonlinux.com/multithreaded-simple-data-type-access-and-atomic-variables
 *  http://www.cnblogs.com/FrankTan/archive/2010/12/11/1903377.html
 *
 *  the demo, create 4 threads. the odd thread do --, the even threads do ++. 
 *  so, the global_int should be zero. we can see:
 *  1. the "no lock version", fastest but not thread safe
 *  2. the "pthread mutex version" slowest, but thread safe
 *  3. the "atomic version", fast and thread safe
 *
 *  *Version:  1.0
 *  *Created:  09/11/2012 05:28:13 PM
 *  *   Revision:  none
 *  *   Compiler:  gcc
 *  *
 *  * Author:  TED (cn), guolb57@163.com
 *  *   
 *  * ===================================================================================
 *  */

 #include <stdio.h>
 #include <pthread.h>
 #include <unistd.h>
 #include <stdlib.h>
 #include <errno.h>

 /************************** for test the time cost begin *********************/
 #include <sys/time.h>

 struct timeval tp1, tp2;

 static void start_timer()
 {
 gettimeofday(&tp1, NULL);

 }

 /* return by seconds */
 static double end_timer()
 {
 gettimeofday(&tp2, NULL);
 return((tp2.tv_sec - tp1.tv_sec) + 
   (tp2.tv_usec - tp1.tv_usec) * 1.0e-6);

 }

 /* for test the time cost end */

 #define LOOP_NUM 1000000

 int global_int = 0;

 #define VER  3  /* modify VER to choose the version here */

 #define NO_LOCK_VER  1  /* quickest & not thread safe */
 #define MUTEX_VER2  /* slowest & thread safe */ 
 #define ATOMIC_VER   3  /* quick & thread safe */

 #if (VER == MUTEX_VER)
 pthread_mutex_t lock;
 #endif

 void *thread_proc( void *arg  )
 {
 int i;
 int flag = *(int*)arg;

 for (i = 0; i < LOOP_NUM; i++){
 if(flag % 2 == 0){
 #if (VER == NO_LOCK_VER)
 global_int++;
 #elif (VER == ATOMIC_VER)
 __sync_fetch_and_add( &global_int, 1  );
 #elif (VER == MUTEX_VER)
 pthread_mutex_lock(&lock);
 global_int++;
 pthread_mutex_unlock(&lock);
 #endif
 
 }
 else{
 #if (VER == NO_LOCK_VER)
 global_int--;
 #elif (VER == ATOMIC_VER)
 __sync_fetch_and_sub(&global_int, 1);
 #elif (VER == MUTEX_VER)
 pthread_mutex_lock(&lock);
 global_int--;
 pthread_mutex_unlock(&lock);
 #endif
 
 }

 }

return NULL;

 }

 int main(int argc, const char *argv[])
 {
int i;
int thread_num = 4;
pthread_t *pthread_id;

 #if (VER == MUTEX_VER)
 pthread_mutex_init(&lock,NULL);
 #endif

 int ntimes = 100;
 start_timer();
 while(ntimes--){
 pthread_id = (pthread_t*)malloc( sizeof( pthread_t  ) * thread_num );
 if (NULL == pthread_id){
 perror( "malloc"  );
 exit(1);
 
 }

 for (i = 0; i < thread_num; i++){
 if (pthread_create( &pthread_id[i], NULL, thread_proc, (void*)&i )){
 perror( "pthread_create"  );
 exit(1);
 
 }
 
 }

 for (i = 0; i < thread_num; i++){
 pthread_join(pthread_id[i], NULL);
 
 }

 free(pthread_id);

 printf( "global_int value is: %d\n", global_int  );
 printf( "The Value should be: 0\n" );

 global_int = 0;
 
 }

 printf("cost %g\n", (double)end_timer());

return 0;

 }


