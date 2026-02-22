#include <stdio.h>
#include <stdbool.h>
#include "queue.h"
// Generated with Gemini

// Initialize the queue to an empty state
void initQueue(Queue *q)
{
    q->front = 0;
    q->rear = -1;
    q->size = 0;
}

// Check if queue is full
bool isFull(Queue *q)
{
    return q->size == MAX_QUEUE_SIZE;
}

// Check if queue is empty
bool isEmpty(Queue *q)
{
    return q->size == 0;
}

// Add an element to the back
void enqueue(Queue *q, int value)
{
    if (isFull(q))
    {
        printf("Queue Error: Cannot enqueue to a full queue.\n");
        return;
    }
    // The modulo operator (%) creates the circular wrap-around
    q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
    q->items[q->rear] = value;
    q->size++;
}

// Remove and return the front element
int dequeue(Queue *q)
{
    if (isEmpty(q))
    {
        printf("Queue Error: Cannot dequeue from an empty queue.\n");
        return -1; // Return an error value
    }
    int value = q->items[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    q->size--;
    return value;
}

// Look at the front element without removing it
int peek(Queue *q)
{
    if (isEmpty(q))
        return -1;
    return q->items[q->front];
}