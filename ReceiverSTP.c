// ReceiverSTP.c
// Implementation of the ReceiverSTP ADT
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#include <err.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Queue.h"
#include "ReceiverLogger.h"
#include "ReceiverSocket.h"
#include "ReceiverSTP.h"
#include "Segment.h"

typedef unsigned int uint;

struct receiverSTP {
	ReceiverSocket rsock;
	ReceiverLogger rlogger;
	
	Queue          rqueue;
	uint           lastSeqNo;
	uint           windowSize;
	char          *dataBuffer;
	uint           dataLength;
	uint           recvBase;
	
	sem_t          canFetch;
	sem_t          received;
	
	pthread_t      receiveDataThread;
	pthread_t      handleDataThread;
};

// Thread functions
static void *receiveData(void *arg);
static void *handleData(void *arg);

// Helper fuctions
static int checksumIsCorrect(Segment s);
static int dupSegmentReceived(int nSegments, Segment buffer[],
                              int recvBase, Segment s);
static void insertInOrder(int nSegments, Segment buffer[], Segment s);
static int getNumInOrderSegments(int nSegments, Segment buffer[]);
static int getInOrderDataLength(int nInOrderSegments, Segment buffer[]);
static void copyToDataBuffer(ReceiverSTP rstp, int nInOrderSegments,
                             Segment buffer[]);

ReceiverSTP newSTP(int recvPort) {
	ReceiverSTP rstp = malloc(sizeof(struct receiverSTP));
	if (rstp == NULL) {
		errx(EXIT_FAILURE, "Insufficient memory!");
	}
	rstp->rsock = newSocket(recvPort);
	
	rstp->rqueue = newQueue();
	rstp->dataBuffer = NULL;
	
	sem_init(&(rstp->canFetch), 0, 0);
	sem_init(&(rstp->received), 0, 0);
	
	return rstp;
}

int pullDataFromSTP(ReceiverSTP rstp, char **dataBuffer) {
	sem_wait(&(rstp->canFetch));
	
	int length = rstp->dataLength;
	char *data = malloc(length * sizeof(char));
	if (data == NULL) {
		errx(EXIT_FAILURE, "Insufficient memory! (pullDataFromSTP)");
	}
	memcpy(data, rstp->dataBuffer, length);
	*dataBuffer = data;
	
	sem_post(&(rstp->received));
	return length;
}

// Thread for receiving data. This is as minimal as possible
// so we don't accidentally miss segments that the sender is
// firing at us.
static void *receiveData(void *arg) {
	ReceiverSTP rstp = (ReceiverSTP)arg;
	uint bufferSize = rstp->windowSize + getHeaderSize();
	Segment s = malloc(bufferSize * sizeof(char));
	if (s == NULL) {
		errx(EXIT_FAILURE, "Insufficient memory! (receiveData)");
	}
	int segmentSize;
	
	// When we receive a segment, make a copy of it and then
	// add it to the segment queue
	while (1) {
		segmentSize = receiveSocket(rstp->rsock, bufferSize, s);
		Segment copy = malloc(segmentSize * sizeof(char));
		if (copy == NULL) {
			errx(EXIT_FAILURE, "Insufficient memory! (receiveData)");
		}
		memcpy(copy, s, segmentSize);
		enterQueue(rstp->rqueue, copy);
	}
	
	return NULL;
}

// Pulls segments off the queue and decides what to do with
// them.
static void *handleData(void *arg) {
	ReceiverSTP rstp = (ReceiverSTP)arg;
	
	int recvBase = 1;
	Segment buffer[rstp->windowSize];
	int nSegments = 0;
	
	while (1) {
		// leaveQueue blocks if theres nothing in the queue
		Segment s = leaveQueue(rstp->rqueue);
		printf("Received: seq no. %d\n", getSeqNo(s));
		// Calculate the  checksum to see if the segment is
		// corrupted
		if (!checksumIsCorrect(s)) {
			logEvent(rstp->rlogger, RECEIVED | CORRUPTED_DATA, s);
			freeSegment(s);
			continue;
		}
		
		// If we received a duplicate segment, ACK recvBase
		if (dupSegmentReceived(nSegments, buffer, recvBase, s)) {
			printf("Duplicate segment received, ACK %d\n", recvBase);
			logEvent(rstp->rlogger, RECEIVED | DUPLICATE_DATA, s);
			freeSegment(s);
			
			Segment ack = newSegment(1, recvBase, 0, 0, ACK, NULL);
			logEvent(rstp->rlogger, SENT | DUPLICATE_ACK, ack);
			replySocket(rstp->rsock, 0, ack);
			freeSegment(ack);
			continue;
		}
		
		// Insert the segment into the buffer in order with
		// insertion sort
		logEvent(rstp->rlogger, RECEIVED, s);
		insertInOrder(nSegments, buffer, s);
		nSegments++;
		
		// If the segment has the next expected byte (i.e.,
		// recvBase)
		if (getSeqNo(s) == recvBase) {
			
			int nInOrderSegments = getNumInOrderSegments(nSegments,
			                                             buffer);
			int dataLength = getInOrderDataLength(nInOrderSegments,
			                                      buffer);
			printf("There are now %d inorder segments, containing "
			       "%d bytes of data in total\n", nInOrderSegments,
			       dataLength);
			
			free(rstp->dataBuffer);
			rstp->dataBuffer = malloc(dataLength);
			copyToDataBuffer(rstp, nInOrderSegments, buffer);
			rstp->dataLength = dataLength;
			
			sem_post(&(rstp->canFetch));
			sem_wait(&(rstp->received));
			
			recvBase = getSeqNo(buffer[nInOrderSegments - 1]) +
			           getDataLength(buffer[nInOrderSegments - 1]);
			if (hasFlag(s, FIN)) recvBase++;
			for (int i = 0; i < nSegments - nInOrderSegments; i++) {
				buffer[i] = buffer[i + nInOrderSegments];
			}
			nSegments -= nInOrderSegments;
			
			printf("ACK %d\n", recvBase);
			Segment ack = newSegment(1, recvBase, 0, 0, ACK, NULL);
			logEvent(rstp->rlogger, SENT, ack);
			replySocket(rstp->rsock, 0, ack);
			freeSegment(ack);
			
		// If the segment is out of order (its sequence no.
		// is greater than recvBase), ACK recvBase
		} else {
			printf("Out of order, ACK %d\n", recvBase);
			
			// Duplicate ACK
			Segment ack = newSegment(1, recvBase, 0, 0, ACK, NULL);
			logEvent(rstp->rlogger, SENT | DUPLICATE_ACK, ack);
			replySocket(rstp->rsock, 0, ack);
			freeSegment(ack);
		}
		
		rstp->recvBase = recvBase;
	}
	
	return NULL;
}

// Check if the checksum is correct
static int checksumIsCorrect(Segment s) {
	return (getChecksum(s) == calcChecksum(s));
}

// Check if a duplicate segment has been received.
static int dupSegmentReceived(int nSegments, Segment buffer[],
                              int recvBase, Segment s) {
    int seqNo = getSeqNo(s);
	if (seqNo < recvBase) {
		return 1;
	}
	for (int i = 0; i < nSegments; i++) {
		if (seqNo == getSeqNo(buffer[i])) {
			return 1;
		}
	}
	return 0;
}

// Inserts a segment into the buffer in order with
// insertion sort
static void insertInOrder(int nSegments, Segment buffer[], Segment s) {
	int seqNo = getSeqNo(s);
	int i;
	for (i = nSegments; i > 0 && getSeqNo(buffer[i - 1]) > seqNo; i--) {
		buffer[i] = buffer[i - 1];
	}
	buffer[i] = s;
}

// Gets the number of inorder segments, so we know
// how many segments worth of data we can send up
// to the application layer
static int getNumInOrderSegments(int nSegments, Segment buffer[]) {
	int nInOrderSegments = 1;
	for (int i = 1; i < nSegments; i++) {
		if (getSeqNo(buffer[i - 1]) + getDataLength(buffer[i - 1]) !=
				getSeqNo(buffer[i])) {
			break;
		}
		nInOrderSegments++;
	}
	return nInOrderSegments;
}

// Gets the total number of bytes of data in the
// first nInOrderSegments segments in the buffer.
// These segments must be in order when the
// function is called.
static int getInOrderDataLength(int nInOrderSegments, Segment buffer[]) {
	int dataLength = 0;
	for (int i = 0; i < nInOrderSegments; i++) {
		dataLength += getDataLength(buffer[i]);
	}
	return dataLength;
}

static void copyToDataBuffer(ReceiverSTP rstp, int nInOrderSegments,
                             Segment buffer[]) {
	int cumulativeLen = 0;
	for (int i = 0; i < nInOrderSegments; i++) {
		memcpy(rstp->dataBuffer + cumulativeLen, getDataPortion(buffer[i]),
		       getDataLength(buffer[i]));
		cumulativeLen += getDataLength(buffer[i]);
	}
}

////////////////////////////////////////////////////////////////////////
// Establish the connection on the receiver side
// through the three-way handshake.
void establishSTP(ReceiverSTP rstp) {
	rstp->rlogger = newReceiverLogger("Receiver_log.txt");

	Segment s;
	
	// Receiving a SYN
	s = malloc(20);
	receiveSocket(rstp->rsock, 20, s);
	logEvent(rstp->rlogger, RECEIVED, s);
	rstp->windowSize = getWindowSize(s);
	freeSegment(s);
	
	// Sending a SYN/ACK
	s = newSegment(0, 1, rstp->windowSize,
	               0, SYN | ACK, NULL);
	logEvent(rstp->rlogger, SENT, s);
	replySocket(rstp->rsock, 0, s);
	freeSegment(s);
	
	// Receiving an ACK
	s = malloc(20);
	receiveSocket(rstp->rsock, 20, s);
	logEvent(rstp->rlogger, RECEIVED, s);
	freeSegment(s);
	
	pthread_create(&(rstp->handleDataThread), NULL, handleData, rstp);
	pthread_create(&(rstp->receiveDataThread), NULL, receiveData, rstp);
}

////////////////////////////////////////////////////////////////////////
// Teardown the connection on the receiver side
// through the four-way connection termination
// Different to the sender's teardown because
// the receiver is waiting for the sender to
// initiate the close.
void teardownSTP(ReceiverSTP rstp) {
	pthread_cancel(rstp->receiveDataThread);
	pthread_cancel(rstp->handleDataThread);
	
	Segment s;
	
	// Waste time
	for (int i = 0; i < 1000; i++) {
		for (int j = 0; j < 1000; j++) {
		}
	}
	
	// Sending a FIN
	s = newSegment(1, rstp->recvBase,
	               rstp->windowSize, 0,
	               FIN, NULL);
	logEvent(rstp->rlogger, SENT, s);
	replySocket(rstp->rsock, 0, s);
	freeSegment(s);
	
	// Receiving an ACK
	s = malloc(20);
	receiveSocket(rstp->rsock, 20, s);
	logEvent(rstp->rlogger, RECEIVED, s);
	freeSegment(s);
	
	logSummary(rstp->rlogger);
	closeSocket(rstp->rsock);
}

