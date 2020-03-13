// SenderSTP.h
// Header file for the SenderSTP ADT

// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for COMP3331 18s2 Assignment

#ifndef SENDER_STP
#define SENDER_STP

#include "SenderLogger.h"

typedef struct senderSTP *SenderSTP;

SenderSTP newSTP(char *recvIp, uint recvPort, uint mws, uint mss, uint gamma,
                 float pDrop, float pDuplicate, float pCorrupt, float pOrder,
                 uint maxOrder, float pDelay, uint maxDelay);

void pushDataToSTP(SenderSTP sstp, uint length, char data[]);

void establishSTP(SenderSTP sstp);

void teardownSTP(SenderSTP sstp);

#endif

