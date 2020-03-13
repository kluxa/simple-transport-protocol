// SenderWindow.c
// Implementation of the SenderWindow ADT
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#include <err.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#include "Segment.h"
#include "SenderWindow.h"

struct window {
	uint     mws;
	uint     mss;
	
	uint     numOccupiedSpaces;
	uint     numSpaces;
	Segment *buffer;
	
	uint     baseIndex;
	
	uint     sendBase;
	uint     lastByteSent;
	uint     nextSeqNo;
	
	sem_t    mutex;
};

SenderWindow newSenderWindow(uint mws, uint mss) {
	SenderWindow window = malloc(sizeof(struct window));
	if (window == NULL) {
		errx(EXIT_FAILURE, "Insufficient memory!");
	}
	
	window->mws = mws;
	window->mss = mss;
	
	window->numOccupiedSpaces = 0;
	window->numSpaces = (mws >= mss ? mws / mss : 1);
	window->buffer = calloc(window->numSpaces, sizeof(Segment));
	
	window->baseIndex =  0;
	
	window->sendBase     = 1;
	window->lastByteSent = 0;
	window->nextSeqNo    = 1;
	
	sem_init(&(window->mutex), 0, 1);
	
	return window;
}

uint getMws(SenderWindow window) {
	return window->mws;
}

uint getSendBase(SenderWindow window) {
	return window->sendBase;
}

uint getLastByteSent(SenderWindow window) {
	return window->lastByteSent;
}

uint getNextSeqNo(SenderWindow window) {
	return window->nextSeqNo;
}

void updateLastByteSent(SenderWindow window, Segment s) {
	window->lastByteSent = getSeqNo(s) + getDataLength(s) - 1;
}

// Creates a segment from the given data and buffers it.
// Also returns a copy of the segment so it can be transmitted.
Segment bufferData(SenderWindow window, int length, char data[]) {
	
	// Wait until there is window space available
	while (window->numOccupiedSpaces == window->numSpaces);
	
	sem_wait(&(window->mutex));
	
	// Create a segment from the data and buffer it in the array
	Segment s = newSegment(window->nextSeqNo, 1, window->mws, length,
	                       0, data);
	Segment copy = duplicateSegment(s);
	window->nextSeqNo += length;
	
	// Figure out the index at which the new segment is inserted,
	// and then insert the segment
	int insertAt = (window->baseIndex + window->numOccupiedSpaces) %
	               window->numSpaces;
	freeSegment(window->buffer[insertAt]);
	window->buffer[insertAt] = s;
	window->numOccupiedSpaces++;
	
	sem_post(&(window->mutex));
	
	return copy;
}

// Slide the window across in response to a new ACK received. Returns
// Returns 1 if there are still any unacknowledged segments,
// or 0 otherwise.
int slideWindow(SenderWindow window, uint ackNo) {
	sem_wait(&(window->mutex));
	
	// Update SendBase
	window->sendBase = ackNo;
	
	// Find out how far to slide the window
	int nSpacesFreed = 0;
	for (int i = 0; i < window->numOccupiedSpaces; i++) {
		int index = (window->baseIndex + i) % window->numSpaces;
		if (getSeqNo(window->buffer[index]) >= ackNo) break;
		nSpacesFreed++;
	}
	window->baseIndex = (window->baseIndex + nSpacesFreed) % window->numSpaces;
	window->numOccupiedSpaces -= nSpacesFreed;
	
	// Check if there are currently any not-yet-acknowledged segments
	int result = 0;
	for (int i = 0; i < window->numOccupiedSpaces; i++) {
		int index = (window->baseIndex + i) % window->numSpaces;
		if (getSeqNo(window->buffer[index]) >= ackNo) {
			result = 1; break;
		}
	}
	
	sem_post(&(window->mutex));
	return result;
}

// Returns a copy of the segment with sequence no. equal to seqNo. It
// is invalid to give this function a sequence no. that is not
// currently covered by the window.
Segment getSegment(SenderWindow window, uint seqNo) {
	sem_wait(&(window->mutex));
	
	Segment s = NULL;
	
	for (int i = 0; i < window->numOccupiedSpaces; i++) {
		int index = (window->baseIndex + i) % window->numSpaces;
		if (getSeqNo(window->buffer[index]) == seqNo) {
			s = duplicateSegment(window->buffer[index]);
			break;
		}
	}
	
	sem_post(&(window->mutex));
	return s;
}

// Returns a copy of the segment with sequence no. equal to sendBase
Segment getBaseSegment(SenderWindow window) {
	sem_wait(&(window->mutex));
	
	Segment s = duplicateSegment(window->buffer[window->baseIndex]);
	
	sem_post(&(window->mutex));
	return s;
}

