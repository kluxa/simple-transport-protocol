// Queue.c
// Implementation of the Queue ADT
// The queue enables proper coordination of threads and allows
// threads to be very localised in their functionality.
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#include "Queue.h"

struct node {
	void        *item;
	struct node *next;
};

struct queue {
	struct node *head;
	struct node *tail;
	int          size;
	sem_t       *lock;
};

Queue newQueue(void) {
	Queue new = calloc(1, sizeof(struct queue));
	new->lock = malloc(sizeof(sem_t));
	sem_init(new->lock, 0, 1);
	return new;
}

void enterQueue(Queue q, void *item) {
	struct node *new = malloc(sizeof(*new));
	new->item = item;
	new->next = NULL;
	
	sem_wait(q->lock);
	
	if (q->size == 0) {
		q->head = new;
	} else {
		q->tail->next = new;
	}
	q->tail = new;
	q->size++;
	
	sem_post(q->lock);
}

void *leaveQueue(Queue q) {
	while (q->size == 0);
	
	sem_wait(q->lock);
	
	struct node *first = q->head;
	q->head = first->next;
	void *item = first->item;
	free(first);
	q->size--;
	
	sem_post(q->lock);
	return item;
}

