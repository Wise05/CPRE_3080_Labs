#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

#define MAX_QUEUE_SIZE 100

// Structure definition for a Circular Queue
typedef struct
{
    int items[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int size;
} Queue;

// Function Prototypes
void initQueue(Queue *q);
bool isFull(Queue *q);
bool isEmpty(Queue *q);
void enqueue(Queue *q, int value);
int dequeue(Queue *q);
int peek(Queue *q);

#endif