// SenderPLD.c
// Implementation of the SenderPLD ADT
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Queue.h"
#include "Segment.h"
#include "SenderLogger.h"
#include "SenderPLD.h"

struct senderPLD {
	float pDrop;
	float pDuplicate;
	float pCorrupt;
	float pOrder;
	uint  maxOrder;
	float pDelay;
	uint  maxDelay;
	
	SegmentToBeSent reordered;
	uint            reorderCount;
};

struct segmentToBeSent {
	Segment s;
	Event   e;
};

// We need these because C doesn't
// support creating threads with
// multiple arguments -_-
typedef struct delayedSegment {
	SenderPLD       pld;
	SenderLogger    logger;
	Queue           queue;
	SegmentToBeSent tbs;
} *DelayedSegment;

static void  checkReorder(SenderPLD pld, SenderLogger logger, Queue queue);
static void *delaySegment(void *arg);
static uint  getRandomDelay(SenderPLD pld);
static float getRandomFloat(void);

SenderPLD newSenderPLD(float pDrop, float pDuplicate, float pCorrupt,
                       float pOrder, uint maxOrder, float pDelay,
                       uint maxDelay) {
	SenderPLD pld = malloc(sizeof(struct senderPLD));
	if (pld == NULL) {
		errx(EXIT_FAILURE, "Insufficient memory!\n");
	}
	
	pld->pDrop = pDrop;
	pld->pDuplicate = pDuplicate;
	pld->pCorrupt = pCorrupt;
	pld->pOrder = pOrder;
	pld->maxOrder = maxOrder;
	pld->pDelay = pDelay;
	pld->maxDelay = maxDelay;
	
	pld->reordered = NULL;
	pld->reorderCount = 0;
	return pld;
}

// 'queue' is where the segment will go after it has gone through
// the PLD module (if it is not dropped)
void fowardToPld(SenderPLD pld, SegmentToBeSent tbs, Queue queue,
                 SenderLogger logger) {
    
    // DROPPED
	if (getRandomFloat() < pld->pDrop) {
		printf("Dropped.\n");
		logEvent(logger, tbs->e | DROPPED, tbs->s);
		checkReorder(pld, logger, queue);
		free(tbs->s); free(tbs);
		return;
	}
	
	// DUPLICATED
	else if (getRandomFloat() < pld->pDuplicate) {
		printf("Duplicated.\n");
		Segment copy = duplicateSegment(tbs->s);
		logEvent(logger, tbs->e, copy);
		enterQueue(queue, copy);
		
		checkReorder(pld, logger, queue);
		
		logEvent(logger, tbs->e | DUPLICATED, tbs->s);
		enterQueue(queue, tbs->s);
		free(tbs);
		
		checkReorder(pld, logger, queue);
	}
	
	// CORRUPTED
	// We corrupt the rightmost bit of the first data byte
	else if (getRandomFloat() < pld->pCorrupt) {
		printf("Corrupted.\n");
		*(getDataPortion(tbs->s)) ^= 1;
		logEvent(logger, tbs->e | CORRUPTED, tbs->s);
		enterQueue(queue, tbs->s);
		free(tbs);
		
		checkReorder(pld, logger, queue);
	}
	
	// REORDERED
	else if (getRandomFloat() < pld->pOrder) {
		// If there is no segment waiting to be reordered
		if (pld->reordered == NULL) {
			printf("Reordered.\n");
			pld->reordered = tbs;
			pld->reorderCount = pld->maxOrder;
			
		// If there is already a segment being reordered
		} else {
			logEvent(logger, tbs->e, tbs->s);
			enterQueue(queue, tbs->s);
			free(tbs);
			
			checkReorder(pld, logger, queue);
		}
	}
	
	// DELAYED
	else if (getRandomFloat() < pld->pDelay) {
		printf("Delayed.\n");
		DelayedSegment ds = malloc(sizeof(struct delayedSegment));
		ds->pld = pld;
		ds->logger = logger;
		ds->queue = queue;
		ds->tbs = tbs;
		
		pthread_t delayThread;
		pthread_create(&delayThread, NULL, delaySegment, ds);
		pthread_detach(delayThread);
	}
	
	// NO ERROR
	else {
		logEvent(logger, tbs->e, tbs->s);
		enterQueue(queue, tbs->s);
		free(tbs);
		checkReorder(pld, logger, queue);
	}
}

// Checks if there is a segment that is being reordered that should be sent now,
// and enqueues the segment if so.
static void checkReorder(SenderPLD pld, SenderLogger logger, Queue queue) {
	if (pld->reorderCount > 0) {
		pld->reorderCount--;
	}
	
	if (pld->reorderCount == 0 && pld->reordered != NULL) {
		logEvent(logger, pld->reordered->e | REORDERED, pld->reordered->s);
		enterQueue(queue, pld->reordered->s);
		free(pld->reordered);
		pld->reordered = NULL;
	}
}

// Thread for subjecting segments to delay
static void *delaySegment(void *arg) {
	DelayedSegment ds = (DelayedSegment)arg;
	
	uint delay = getRandomDelay(ds->pld);
	struct timespec duration = {delay / 1000, (delay % 1000) * 1000000};
	nanosleep(&duration, NULL);
	
	logEvent(ds->logger, ds->tbs->e | DELAYED, ds->tbs->s);
	enterQueue(ds->queue, ds->tbs->s);
	
	checkReorder(ds->pld, ds->logger, ds->queue);
	
	free(ds->tbs);
	free(ds);
	pthread_exit(NULL);
	return NULL;
}

// Returns a random delay in the range [0, maxDelay] in milliseconds
static uint getRandomDelay(SenderPLD pld) {
	return (rand() % (pld->maxDelay + 1));
}

// Returns a random number between 0 and 1
static float getRandomFloat(void) {
	return rand()/((float)(RAND_MAX) + 1);
}

