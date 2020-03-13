// SenderLogger.c
// Implementation of the SenderLogger ADT
// The SenderLogger ADT logs events and maintains statistics
// for the sender
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#include <err.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "SenderLogger.h"
#include "Segment.h"

typedef unsigned int uint;

struct senderLogger {
	FILE          *log;
	sem_t         *lock;
	struct timeval start;
	
	uint fileSize;
	uint numSegmentsTransmitted; // Including drop and RXT
	
	uint numPldSegments;
	uint numSegmentsDropped;
	uint numSegmentsDuplicated;
	uint numSegmentsCorrupted;
	uint numSegmentsReordered;
	uint numSegmentsDelayed;
	
	uint numDuplicateAcksReceived;
	uint numTimeoutRetransmits;
	uint numFastRetransmits;
};

static double getTime(SenderLogger logger);
static void updateStatistics(SenderLogger logger, Event e, Segment s);
static double timeDiff(struct timeval t0, struct timeval t1);
static char *eventToString(char *buffer, Event e);

SenderLogger newSenderLogger(char *filename) {
	SenderLogger new = calloc(1, sizeof(struct senderLogger));
	if (new == NULL) {
		errx(EXIT_FAILURE, "Insufficient memory!\n");
	}
	
	new->log = fopen(filename, "w");
	new->lock = malloc(sizeof(sem_t));
	
	// Print out a header line
	fprintf(new->log, "%-15s%11s%10s%-4s%10s%10s%10s\n",
	        "event", "time", "", "type", "seq", "data",
	        "ack");
	
	// setbuf(new->log, NULL);
	sem_init(new->lock, 0, 1);
	
	// We take the time when the log
	// is instantiated to be time 0.
	gettimeofday(&(new->start), NULL);
	
	return new;
}

void logEvent(SenderLogger logger, Event e, Segment s) {
	sem_wait(logger->lock);
	
	double time = getTime(logger);	
	char event[20] = {0};
	char flags[20] = {0};
	
	fprintf(logger->log, "%-15s%11.5lf%10s%-4s%10d%10d%10d\n",
	        eventToString(event, e), time, "", getFlags(s, flags),
	        getSeqNo(s), getDataLength(s), getAckNo(s));
	updateStatistics(logger, e, s);
	
	sem_post(logger->lock);
}

void updateFileSizeStatistic(SenderLogger logger, uint dataSize) {
	logger->fileSize += dataSize;
}

static void updateStatistics(SenderLogger logger, Event e, Segment s) {
	if (e & SENT) {
		logger->numSegmentsTransmitted++;
		
		if (getDataLength(s) > 0) {
			logger->numPldSegments++;
		}
		
		if (e & DROPPED) {
			logger->numSegmentsDropped++;
		} else if (e & DUPLICATED) {
			logger->numSegmentsDuplicated++;
		} else if (e & CORRUPTED) {
			logger->numSegmentsCorrupted++;
		} else if (e & REORDERED) {
			logger->numSegmentsReordered++;
		} else if (e & DELAYED) {
			logger->numSegmentsDelayed++;
		}
		
		if (e & TIMEOUT_REXMIT) {
			logger->numTimeoutRetransmits++;
		} else if (e & FAST_REXMIT) {
			logger->numFastRetransmits++;
		}
		
	} else if (e & RECEIVED) {
		if (e & DUPLICATE_ACK) {
			logger->numDuplicateAcksReceived++;
		}
	}
}

void logSummary(SenderLogger logger) {
	fprintf(logger->log,
		"\n"
		"=========================================================\n"
		"Size of the file (in bytes)                %14d\n"
		"Segments transmitted (including drop & RXT)%14d\n"
		"Number of segments handled by PLD          %14d\n"
		"Number of segments dropped                 %14d\n"
		"Number of segments duplicated              %14d\n"
		"Number of segments corrupted               %14d\n"
		"Number of segments reordered               %14d\n"
		"Number of segments delayed                 %14d\n"
		"Number of retransmissions due to TIMEOUT   %14d\n"
		"Number of FAST RETRANSMISSIONs             %14d\n"
		"Number of DUP ACKs received                %14d\n"
		"=========================================================\n"
		"\n",
		
		logger->fileSize,
		logger->numSegmentsTransmitted,
		logger->numPldSegments,
		logger->numSegmentsDropped,
		logger->numSegmentsDuplicated,
		logger->numSegmentsCorrupted,
		logger->numSegmentsReordered,
		logger->numSegmentsDelayed,
		logger->numTimeoutRetransmits,
		logger->numFastRetransmits,
		logger->numDuplicateAcksReceived
	);
	
	fclose(logger->log);
}

static double getTime(SenderLogger logger) {
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
		
		if (e & RETRANSMITTED) {
			strcat(buffer, "/RXT");
		}
		
		if (e & DROPPED) {
			strcat(buffer, "/drop");
		} else if (e & DUPLICATED) {
			strcat(buffer, "/dup" );
		} else if (e & CORRUPTED) {
			strcat(buffer, "/corr");
		} else if (e & REORDERED) {
			strcat(buffer, "/rord");
		} else if (e & DELAYED) {
			strcat(buffer, "/dely");
		}
		
	} else if (e & RECEIVED) {
		strcat(buffer, "rcv");
		
		if (e & DUPLICATE_ACK) {
			strcat(buffer, "/DA");
		}
	}
	
	return buffer;
}

