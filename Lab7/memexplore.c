#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>


int global_initialized = 42;
int global_uninitialized;

int main() {
    int stack_var = 10;
    void *malloc_ptr = malloc(4096); 
    
    void *mmap_ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, 
                          MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (malloc_ptr == NULL || mmap_ptr == MAP_FAILED) {
        perror("Allocation failed");
        return 1;
    }

    printf("--- Memory Exploration ---\n");
    printf("PID: %d\n\n", getpid());

    printf("Global (Initialized) address:   %p\n", (void*)&global_initialized);
    printf("Global (Uninitialized) address: %p\n", (void*)&global_uninitialized);
    printf("Stack variable address:         %p\n", (void*)&stack_var);
    printf("Malloc (Heap) address:          %p\n", malloc_ptr);
    printf("Mmap (Mapping) address:         %p\n", mmap_ptr);

    printf("\nSleeping for 60 seconds...\n");
    printf("Run 'cat /proc/%d/maps' in another terminal to see the layout.\n", getpid());
    
    sleep(60);

    free(malloc_ptr);
    munmap(mmap_ptr, 4096);

    return 0;
}
