// SenderSTP.c
// Implementation of the SenderSTP ADT
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#include <err.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Queue.h"
#include "Segment.h"
#include "SenderLogger.h"
#include "SenderPLD.h"
#include "SenderSocket.h"
#include "SenderSTP.h"
#include "SenderWindow.h"
#include "Timer.h"

#define TRUE  1
#define FALSE 0

typedef unsigned int uint;

struct senderSTP {
	SenderSocket ssock;
	SenderLogger slogger;
	SenderPLD    spld;
	
	SenderWindow window;
	
	Timer        timer;
	
	uint         reorderCounter;
	
	pthread_t    sendToPldThread;
	pthread_t    receiveAcksThread;
	pthread_t    handleAcksThread;
	pthread_t    transmitThread;
	pthread_t    timerThread;
	
	sem_t        runTimer;
	sem_t        timerLock;
	
	Queue        waitingToBeSent;
	Queue        toBeTransmitted;
	Queue        acksQueue;
};

typedef struct segmentToBeSent {
	Segment   s;
	Event     e;
} *SegmentToBeSent;

static void *sendSegments(void *arg);
static void *xmitSegments(void *arg);

static void *runTimer(void *arg);
static void tryToStartTimer(SenderSTP sstp);
static void stopTimer(SenderSTP sstp);

static void *receiveAcks(void *arg);
static void *handleAcks(void *arg);

SenderSTP newSTP(char *recvIp, uint recvPort, uint mws, uint mss, uint gamma,
                 float pDrop, float pDuplicate, float pCorrupt, float pOrder,
                 uint maxOrder, float pDelay, uint maxDelay) {
	
	SenderSTP sstp = malloc(sizeof(struct senderSTP));
	if (sstp == NULL) {
		errx(EXIT_FAILURE, "Insufficient memory!");
	}
	sstp->ssock = newSocket(recvIp, recvPort);
	
	sstp->spld = newSenderPLD(pDrop, pDuplicate, pCorrupt, pOrder, maxOrder,
	                          pDelay, maxDelay);
	
	sstp->window = newSenderWindow(mws, mss);
	
	sstp->timer = newTimer(gamma);
	
	sem_init(&(sstp->runTimer), 0, 0);
	sem_init(&(sstp->timerLock), 0, 1);
	
	sstp->waitingToBeSent = newQueue();
	sstp->toBeTransmitted = newQueue();
	sstp->acksQueue = newQueue();
	
	return sstp;
}

static SegmentToBeSent newSegmentToBeSent(Segment s, Event e) {
	SegmentToBeSent new = malloc(sizeof(struct segmentToBeSent));
	new->s = s;
	new->e = e;
	return new;
}

////////////////////////////////////////////////////////////////////////
//                                                                    //
//         *******  *   *  ****   *****   ***   ****    ****          //
//            *     *   *  *   *  *      *   *  *   *  *              //
//            *     *****  ****   *****  *****  *   *   ***           //
//            *     *   *  *  *   *      *   *  *   *      *          //
//            *     *   *  *   *  *****  *   *  ****   ****           //
//                                                                    //
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Sending Segments

void pushDataToSTP(SenderSTP sstp, uint length, char data[]) {
	Segment s = bufferData(sstp->window, length, data);
	SegmentToBeSent tbs = newSegmentToBeSent(s, SENT); 
	updateFileSizeStatistic(sstp->slogger, length);
	enterQueue(sstp->waitingToBeSent, tbs);
}

// Thread for sending segments to the PLD
static void *sendSegments(void *arg) {
	SenderSTP sstp = (SenderSTP)arg;
	
	while (1) {
		// Grab next segment to be sent off the queue
		SegmentToBeSent tbs = leaveQueue(sstp->waitingToBeSent);
		tbs->e |= SENT;
		
		// Determine if this segment is being retransmitted
		int rexmit = (getSeqNo(tbs->s) <= getLastByteSent(sstp->window));
		
		// If the segment is not being retransmitted,  update the
		// lastByteSent. If we are currently not sampling the RTT
		// for a segment, start sampling the RTT.
		if (!rexmit) {
			updateLastByteSent(sstp->window, tbs->s);
			if (!isSamplingRTT(sstp->timer)) {
				printf("Starting a sampling of segment with seq no. %d\n", getSeqNo(tbs->s));
				startSamplingRTT(sstp->timer, tbs->s);
			}
		}
		
		// Start the RTO (if it has not already been started) and
		// forward the segment to the PLD module.
		tryToStartTimer(sstp);
		fowardToPld(sstp->spld, tbs, sstp->toBeTransmitted, sstp->slogger);
		
	}
	
	return NULL;
}

// Thread for transmitting segments
static void *xmitSegments(void *arg) {
	SenderSTP sstp = (SenderSTP)arg;
	
	while (1) {
		Segment s = leaveQueue(sstp->toBeTransmitted);
		sendSocket(sstp->ssock, getDataLength(s), s);
		freeSegment(s); // C memory management :(
	}
	
	return NULL;
}

// Thread for maintaining the timer
static void *runTimer(void *arg) {
	SenderSTP sstp = (SenderSTP)arg;
	
	while (1) {
		sem_wait(&(sstp->runTimer));
		printf("Starting timer...\n");
		double timeRemaining = startTimer(sstp->timer);
		
		// If there is a timeout...
		if (timeRemaining == 0) {
			printf("Timeout (RTO was %lf)\n", getTimeOutInterval(sstp->timer));
			Segment s = getBaseSegment(sstp->window);
			cancelSamplingRTT(sstp->timer);
			
			SegmentToBeSent tbs;
			tbs = newSegmentToBeSent(s, TIMEOUT_REXMIT);
			enterQueue(sstp->waitingToBeSent, tbs);
			tryToStartTimer(sstp);
		}
	}
	
	return NULL;
}

static void tryToStartTimer(SenderSTP sstp) {
	int sval;
	sem_wait(&(sstp->timerLock));
	sem_getvalue(&(sstp->runTimer), &sval);
	if (sval == 0) {
		sem_post(&(sstp->runTimer));
	}
	sem_post(&(sstp->timerLock));
}

static void stopTimer(SenderSTP sstp) {
	pthread_kill(sstp->timerThread, SIGALRM);
}

////////////////////////////////////////////////////////////////////////
// Receiving ACKs

// Thread for receiving ACKs
// This is as minimal as possible so we don't miss any ACKs
// from the receiver
static void *receiveAcks(void *arg) {
	SenderSTP sstp = (SenderSTP)arg;
	Segment s = malloc(20);
	int segmentSize;
	
	while (1) {
		segmentSize = socketGetReply(sstp->ssock, s);
		Segment copy = malloc(segmentSize * sizeof(char));
		memcpy(copy, s, segmentSize); // Copy the segment
		enterQueue(sstp->acksQueue, copy);
	}
	
	return NULL;
}

// Thread for handling ACKs
// Grabs ACK segments off the queue one at a time, and then
// handles them accordingly
static void *handleAcks(void *arg) {
	SenderSTP sstp = (SenderSTP)arg;
	int numDuplicateAcks = 0;
	int duplicateAck = 0;
	
	while (1) {
		Segment s = leaveQueue(sstp->acksQueue);
		uint ackNo = getAckNo(s);
		printf("Received ACK %d ", ackNo);
		
		// If a new ACK was received
		if (ackNo > getSendBase(sstp->window)) {
			printf("(NEW)\n");
			if (isSamplingRTT(sstp->timer) &&
					(ackNo > getSampledSeqNo(sstp->timer))) {
				printf("Taking a sample of RTT, ack no. is %d\n", ackNo);
				stopSamplingRTT(sstp->timer);
			}
			
			numDuplicateAcks = 0;
			
			stopTimer(sstp);
			logEvent(sstp->slogger, RECEIVED, s);
			// Update the sender window
			if (slideWindow(sstp->window, ackNo))  {
				printf("There are still unacked segments\n");
				tryToStartTimer(sstp);
			}
		}
		
		// If a duplicate ACK was received
		else {
			printf("(DUPLICATE)\n");
			
			logEvent(sstp->slogger, RECEIVED | DUPLICATE_ACK, s);
			
			// Ignore ACKs below the window
			if (ackNo == getSendBase(sstp->window)) {
				if (ackNo > duplicateAck) {
					numDuplicateAcks = 0;
					duplicateAck = ackNo;
				}
				numDuplicateAcks++;
				
				// Fast retransmit
				if (numDuplicateAcks == 3) {
					numDuplicateAcks = 0;  // Reset duplicate ACK count to zero
					
					// If the duplicate ACK number is the same as the  sequence
					// number of the segment we are using to sample RTT, cancel
					// the sampling of the RTT
					cancelSamplingRTT(sstp->timer);
					
					// Get the segment that has the same sequence number as the
					// duplicate ACK number
					Segment retransmitted = getSegment(sstp->window, ackNo);
					if (retransmitted == NULL) {
						continue;
					}
					
					// Enqueue this segment on the queue of segments to be sent
					SegmentToBeSent tbs = newSegmentToBeSent(retransmitted,
						                                     FAST_REXMIT);
					enterQueue(sstp->waitingToBeSent, tbs);
				}
			}
		}
	}
	
	return NULL;
}

////////////////////////////////////////////////////////////////////////
// Establish the connection on the sender's side
// through the three-way handshake.
// Also initiate the logger.
void establishSTP(SenderSTP sstp) {
	sstp->slogger = newSenderLogger("Sender_log.txt");
	
	Segment s;
	
	// Sending a SYN
	s = newSegment(0, 0, getMws(sstp->window),
	               0, SYN, NULL);
	logEvent(sstp->slogger, SENT, s);
	sendSocket(sstp->ssock, 0, s);
	freeSegment(s);
	
	// Receiving a SYN/ACK
	s = malloc(20);
	socketGetReply(sstp->ssock, s);
	logEvent(sstp->slogger, RECEIVED, s);
	freeSegment(s);
	
	// Sending an ACK
	s = newSegment(1, 1, getMws(sstp->window),
	               0, ACK, NULL);
	logEvent(sstp->slogger, SENT, s);
	sendSocket(sstp->ssock, 0, s);
	freeSegment(s);
	
	// Initiate threads
	pthread_create(&(sstp->transmitThread), NULL, xmitSegments, sstp);
	pthread_create(&(sstp->sendToPldThread), NULL, sendSegments, sstp);
	pthread_create(&(sstp->receiveAcksThread), NULL, receiveAcks, sstp);
	pthread_create(&(sstp->handleAcksThread), NULL, handleAcks, sstp);
	pthread_create(&(sstp->timerThread), NULL, runTimer, sstp);
}

////////////////////////////////////////////////////////////////////////
// Teardown the connection on the sender's side
// through the four-step connection termination
void teardownSTP(SenderSTP sstp) {
	while (getSendBase(sstp->window) != getNextSeqNo(sstp->window));
	
	// Terminate threads
	pthread_cancel(sstp->timerThread);
	pthread_cancel(sstp->transmitThread);
	pthread_cancel(sstp->handleAcksThread);
	pthread_cancel(sstp->receiveAcksThread);
	pthread_cancel(sstp->sendToPldThread);
	
	// Waste time
	for (int i = 0; i < 1000; i++) {
		for (int j = 0; j < 1000; j++) {
		}
	}
	
	Segment s;
	uint nextSeqNo = getNextSeqNo(sstp->window);
	
	// Sending a FIN
	s = newSegment(nextSeqNo++, 1,
	               getMws(sstp->window),
	               0, FIN, NULL);
	logEvent(sstp->slogger, SENT, s);
	sendSocket(sstp->ssock, 0, s);
	freeSegment(s);
	
	// Receiving an ACK
	s = malloc(20);
	socketGetReply(sstp->ssock, s);
	logEvent(sstp->slogger, RECEIVED, s);
	freeSegment(s);
	
	printf("Waiting for the FIN\n");
	
	// Receiving a FIN
	s = malloc(20);
	socketGetReply(sstp->ssock, s);
	logEvent(sstp->slogger, RECEIVED, s);
	freeSegment(s);
	
	printf("Received the FIN\n");
	
	// Waste time
	for (int i = 0; i < 1000; i++) {
		for (int j = 0; j < 1000; j++) {
		}
	}
	
	// Sending an ACK
	s = newSegment(nextSeqNo, 2,
	               getMws(sstp->window),
	               0, ACK, NULL);
	logEvent(sstp->slogger, SENT, s);
	sendSocket(sstp->ssock, 0, s);
	freeSegment(s);
	
	logSummary(sstp->slogger);
	closeSocket(sstp->ssock);
}

