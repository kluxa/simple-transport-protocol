// Segment.h
// Header file for the Segment ADT
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#ifndef SEGMENT_H
#define SEGMENT_H

#define ACK 0x1
#define SYN 0x2
#define FIN 0x4

typedef unsigned int uint;

typedef struct segment *Segment;

Segment newSegment(uint seqNo, uint ackNo, uint windowSize,
                   uint dataLength, unsigned short flags,
                   char buffer[]);

Segment duplicateSegment(Segment s);

uint getHeaderSize(void);

uint getSeqNo(Segment s);

uint getAckNo(Segment s);

uint getWindowSize(Segment s);

uint getDataLength(Segment s);

char *getFlags(Segment s, char *str);

uint hasFlag(Segment s, uint flag);

unsigned short getChecksum(Segment s);

unsigned short calcChecksum(Segment s);

char *getDataPortion(Segment s);

void showSegment(Segment s);

void freeSegment(Segment s);

#endif

