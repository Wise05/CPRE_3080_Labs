# include <stdio.h>
# include <unistd.h>
# include <sys/wait.h>

int main (void) {
printf ( " Parent PID :%d\n" , getpid () ) ;

for ( int i = 0; i < 4; i ++) {
pid_t pid = fork () ;
 if ( pid == 0) {
 // Child process
 printf ( " Child %d : PID =%d , PPID =%d\n" ,
 i , getpid () , getppid () ) ;
 sleep (5) ; // Keep alive for inspection
 return 0;
 }
 }

 // Parent waits for all children
 sleep (5) ; // Keep alive for inspection
 for ( int i = 0; i < 4; i ++) {
 wait ( NULL ) ;
}

 return 0;
}
