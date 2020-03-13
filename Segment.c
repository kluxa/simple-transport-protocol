// Segment.c
// Implementation of the Segment ADT
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Segment.h"

typedef unsigned int uint;

struct segment {
	uint seqNo;
	uint ackNo;
	uint windowSize;
	uint dataLength;
	unsigned short flags;
	unsigned short checksum;
	char data[];
};

Segment newSegment(uint seqNo, uint ackNo, uint windowSize,
                   uint dataLength, unsigned short flags,
                   char buffer[]) {
	Segment s = malloc(sizeof(struct segment) + dataLength);
	if (s == NULL) {
		errx(EXIT_FAILURE, "Insufficient memory! (newSegment)\n");
	}
	
	s->seqNo = seqNo;
	s->ackNo = ackNo;
	s->windowSize = windowSize;
	s->dataLength = dataLength;
	s->flags = flags;
	memcpy(s->data, buffer, dataLength);
	
	s->checksum = calcChecksum(s);
	
	return s;
}

// Assumes the segment is uncorrupted, and has a correct
// dataLength header
Segment duplicateSegment(Segment s) {
	uint segmentSize = getHeaderSize() + s->dataLength;
	Segment copy = malloc(segmentSize * sizeof(char));
	memcpy(copy, s, segmentSize);
	return copy;
}

uint getHeaderSize(void) {
	return sizeof(struct segment);
}

uint getSeqNo(Segment s) {
	return s->seqNo;
}

uint getAckNo(Segment s) {
	return s->ackNo;
}

uint getWindowSize(Segment s) {
	return s->windowSize;
}

uint getDataLength(Segment s) {
	return s->dataLength;
}

char *getFlags(Segment s, char *str) {
	if (s->flags & SYN) {
		strcat(str, "S");
	}
	if (s->flags & FIN) {
		strcat(str, "F");
	}
	if (s->flags & ACK) {
		strcat(str, "A");
	}
	if (s->flags == 0) {
		strcat(str, "D");
	}
	return str;
}

uint hasFlag(Segment s, uint flag) {
	return (s->flags & flag);
}

unsigned short getChecksum(Segment s) {
	return s->checksum;
}

unsigned short calcChecksum(Segment s) {
	unsigned short checksum = 0;
	char *end = (char *)s + getHeaderSize() + getDataLength(s);
	
	for (char *curr = (char *)s; curr != end; curr++) {
		if (curr == (char *)(&(s->checksum))) {
			curr++;
		} else {
			for (char mask = 1; mask > 0; mask <<= 1) {
				if (*curr & mask) {
					checksum = ~checksum;
				}
			}
		}
	}
	
	return ((checksum == 0) ? 0 : 1);
}

char *getDataPortion(Segment s) {
	return (s->data);
}

void showSegment(Segment s) {
	printf("\n=========================================\n");
	printf("Sequence number: %d\n", s->seqNo);
	printf("Acknowledgement number: %d\n", s->ackNo);
	printf("Window size: %d\n", s->windowSize);
	printf("Data length: %d\n", s->dataLength);
	printf("Flags:");
	if (s->flags & ACK) {
		printf(" ACK");
	}
	if (s->flags & SYN) {
		printf(" SYN");
	}
	if (s->flags & FIN) {
		printf(" FIN");
	}
	printf("\n=========================================\n");
}

void freeSegment(Segment s) {
	free(s);
}

