#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void* worker (void* arg) {
long id = ( long ) arg ;
printf ( "Thread %ld : TID =%lu\n" ,
id , ( unsigned long ) pthread_self () ) ;
sleep (5) ; // Keep alive for inspection
 return NULL ;
 }

 int main (void) {
 printf ("Main PID : %d\n" , getpid () ) ;

 pthread_t threads [4];
 for ( long i = 0; i < 4; i ++) {
 pthread_create (& threads [ i ] , NULL , worker , (void *) i ) ;
 }

 sleep (5) ; // Keep process alive for inspection

 for ( int i = 0; i < 4; i ++) {
 pthread_join ( threads [ i ] , NULL ) ;
 }

 return 0;
 }
