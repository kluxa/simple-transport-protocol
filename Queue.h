// Queue.h
// Header file for the Queue ADT
// The queue enables proper coordination of threads
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#ifndef QUEUE_H
#define QUEUE_H

typedef struct queue *Queue;

Queue newQueue(void);

void enterQueue(Queue q, void *item);

void *leaveQueue(Queue q);

#endif

