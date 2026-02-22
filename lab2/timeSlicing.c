#include <stdio.h>
#include <unistd.h>

int main() {
    int i=0, j=0, pid;
    pid = fork();
    if (pid == 0) {
        for(i=0; i<10000000; i++)
            printf("Child: %d\n", i);
    } else {
        for(j=0; j<10000000; j++)
            printf("Parent: %d\n", j);
    }
}