// SenderWindow.h
// Header file for the SenderWindow ADT
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#ifndef SENDER_WINDOW
#define SENDER_WINDOW

#include "Segment.h"

typedef struct window *SenderWindow;

typedef unsigned int uint;

SenderWindow newSenderWindow(uint mws, uint mss);

uint getMws(SenderWindow window);

uint getSendBase(SenderWindow window);

uint getLastByteSent(SenderWindow window);

uint getNextSeqNo(SenderWindow window);

void updateLastByteSent(SenderWindow window, Segment s);

Segment bufferData(SenderWindow window, int length, char data[]);

int slideWindow(SenderWindow window, uint ackNo);

Segment getSegment(SenderWindow window, uint ackNo);

Segment getBaseSegment(SenderWindow window);

#endif

