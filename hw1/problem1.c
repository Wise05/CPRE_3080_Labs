#include <stdio.h>
#include <stdlib.h>

int global_var = 42;

void level3(int *ptr) {
    int local3 = 300;
    printf("L3: &local3=%p, &ptr_on_stack=%p, ptr_points_to=%p, *ptr=%d\n",
           (void *)&local3, (void *)&ptr, (void *)ptr, *ptr);
}

void level2(int x) {
    int local2 = 200;
    printf("L2: &local2=%p, &x_arg=%p\n", (void *)&local2, (void *)&x);
    level3(&local2);
}

void level1(void) {
    int local1 = 100;
    printf("L1: &local1=%p\n", (void *)&local1);
    level2(local1);
}

int main(void) {
    int *heap_var = malloc(sizeof(int));
    if (heap_var == NULL) return 1;
    *heap_var = 500;

    printf("--- Memory Layout Investigation ---\n");
    
    printf("Main Function (Text): %p\n", (void *)main);
    
    printf("Global Var (Data):    %p\n", (void *)&global_var);
    
    printf("Heap Var (Heap):      %p\n", (void *)heap_var);
    
    printf("\n--- Stack Sequence ---\n");
    level1();

    free(heap_var); 
    return 0;
}
