// ReceiverSocket.h
// Header file for the ReceiverSocket ADT

// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for COMP3331 18s2 Assignment

#include "Segment.h"

typedef struct receiverSocket *ReceiverSocket;

ReceiverSocket newSocket(int recvPort);

int receiveSocket(ReceiverSocket rsock, int length, Segment s);

void replySocket(ReceiverSocket rsock, int length, Segment s);

void closeSocket(ReceiverSocket rsock);

