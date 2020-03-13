// SenderSocket.h
// Header file for the SenderSocket ADT
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#include "Segment.h"

typedef struct senderSocket *SenderSocket;

SenderSocket newSocket(char *recvIp, int recvPort);

void sendSocket(SenderSocket ssock, int length, Segment s);

int socketGetReply(SenderSocket ssock, Segment s);

void closeSocket(SenderSocket ssock);

