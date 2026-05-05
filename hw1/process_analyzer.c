#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>

int main() {
    struct timespec start, end;
    pid_t pid;

    // Capture start time
    clock_gettime(CLOCK_MONOTONIC, &start);

    pid = fork();

    if (pid < 0) {
        // 5. Handle fork() errors properly
        fprintf(stderr, "Fork failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    } 
    
    if (pid == 0) {
        // --- Child Process ---
        printf("[Child] Executing 'ls -la /proc/self/'...\n");
        
        // 2. Execute using the exec family (execvp handles path searching)
        char *args[] = {"ls", "-la", "/proc/self/", NULL};
        execvp(args[0], args);

        // If execvp returns, an error occurred
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } 
    else {
        // --- Parent Process ---
        int status;
        
        // 3a. Wait for the child to complete
        waitpid(pid, &status, 0);

        // Capture end time
        clock_gettime(CLOCK_MONOTONIC, &end);

        // Calculate execution time in milliseconds
        // Formula: (Seconds * 1000) + (Nanoseconds / 1,000,000)
        double elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                           (end.tv_nsec - start.tv_nsec) / 1000000.0;

        printf("\n--- Analysis Complete ---\n");
        
        // 3b. Print total execution time
        printf("Total execution time: %.3f ms\n", elapsed_ms);

        // 3c & 6. Print child's exit status using WEXITSTATUS
        if (WIFEXITED(status)) {
            printf("Child exited normally with status: %d\n", WEXITSTATUS(status));
        } else {
            printf("Child terminated abnormally.\n");
        }
    }

    return EXIT_SUCCESS;
}
