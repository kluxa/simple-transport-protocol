// ReceiverLogger.c
// Implementation of the ReceiverLogger ADT
// The ReceiverLogger ADT logs events and maintain statistics
// for the receiver
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#include <err.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "ReceiverLogger.h"
#include "Segment.h"

typedef unsigned int uint;

struct receiverLogger {
	FILE          *log;
	sem_t          lock;
	struct timeval start;
	
	uint amountDataReceived;
	uint numSegmentsReceived;
	uint dataSegmentsReceived;
	uint numCorruptedSegments;
	uint numDuplicateSegments;
	uint numDuplicateAcksSent;
};

static double getTime(ReceiverLogger logger);
static double timeDiff(struct timeval t0, struct timeval t1);
static char *eventToString(char *buffer, Event e);

ReceiverLogger newReceiverLogger(char *filename) {
	ReceiverLogger new = calloc(1, sizeof(struct receiverLogger));
	if (new == NULL) {
		errx(EXIT_FAILURE, "Insufficient memory!\n");
	}
	
	new->log = fopen(filename, "w");
	
	fprintf(new->log, "%-15s%11s%10s%-4s%10s%10s%10s\n",
	        "event", "time", "", "type", "seq", "data",
	        "ack");
	
	// setbuf(new->log, NULL);
	sem_init(&(new->lock), 0, 1);
	gettimeofday(&(new->start), NULL);
	
	return new;
}

void logEvent(ReceiverLogger logger, Event e, Segment s) {
	sem_wait(&(logger->lock));
	
	double time = getTime(logger);	
	char event[20] = {0};
	char flags[20] = {0};
	
	fprintf(logger->log, "%-15s%11.5lf%10s%-4s%10d%10d%10d\n",
	        eventToString(event, e), time, "", getFlags(s, flags),
	        getSeqNo(s), getDataLength(s), getAckNo(s));
	
	if (e & RECEIVED) {
		int dataLength = getDataLength(s);
		if (dataLength > 0) {
			logger->amountDataReceived += dataLength;
			logger->dataSegmentsReceived++;
		}
		logger->numSegmentsReceived++;
		if (e & CORRUPTED_DATA) {
			logger->numCorruptedSegments++;
		}
		if (e & DUPLICATE_DATA) {
			logger->numDuplicateSegments++;
		}
	} else if (e & SENT) {
		if (e & DUPLICATE_ACK) {
			logger->numDuplicateAcksSent++;
		}
	}
	
	sem_post(&(logger->lock));
}

void logSummary(ReceiverLogger logger) {
	fprintf(logger->log,
		"\n"
		"==============================================\n"
		"Amount of data received (bytes) %14d\n"
		"Total segments received         %14d\n"
		"Data segments received          %14d\n"
		"Data segments with bit errors   %14d\n"
		"Duplicate data segments received%14d\n"
		"Duplicate ACKs sent             %14d\n"
		"==============================================\n"
		"\n",
		
		logger->amountDataReceived,
		logger->numSegmentsReceived,
		logger->dataSegmentsReceived,
		logger->numCorruptedSegments,
		logger->numDuplicateSegments,
		logger->numDuplicateAcksSent
	);
	
	fclose(logger->log);
}

static double getTime(ReceiverLogger logger) {
	struct timeval now;
	gettimeofday(&now, NULL);
	
	return timeDiff(logger->start, now);
}

static double timeDiff(struct timeval t0, struct timeval t1) {
	return (t1.tv_usec - t0.tv_usec) / 1000000.0f +
	       (t1.tv_sec - t0.tv_sec);
}

static char *eventToString(char *buffer, Event e) {
	if (e & SENT) {
		strcat(buffer, "snd");
		
		if (e & DUPLICATE_ACK) {
			strcat(buffer, "/DA");
		}
		
	} else if (e & RECEIVED) {
		strcat(buffer, "rcv");
		
		if (e & CORRUPTED_DATA) {
			strcat(buffer, "/corr");
		}
	}
	
	return buffer;
}

