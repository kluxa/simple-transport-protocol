// SenderPLD.h
// Header file for the SenderPLD ADT
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#ifndef SENDER_PLD
#define SENDER_PLD

typedef struct senderPLD *SenderPLD;

typedef unsigned int uint;

typedef struct segmentToBeSent *SegmentToBeSent;

SenderPLD newSenderPLD(float pDrop, float pDuplicate, float pCorrupt,
                       float pOrder, uint maxOrder, float pDelay,
                       uint maxDelay);

void fowardToPld(SenderPLD pld, SegmentToBeSent tbs, Queue queue,
                 SenderLogger logger);

#endif

