#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main (void) {
  for ( int i = 0; i < 3; i ++) {
    pid_t rc = fork () ;
    if ( rc == 0) {
       printf ( "Child %d done (pid =%d , ppid =%d)\n" , i , getpid() ,
      getppid() ) ;
      exit (0) ;
    }
  }

  for (int i = 0; i < 3; i++) {
      wait(NULL); 
  }

  printf ("Parent going to sleep ... (pid =%d)\n" , getpid() ) ;
  sleep (10) ;
  printf ( "Parent waking up ...\n" ) ;
  return 0;
}
