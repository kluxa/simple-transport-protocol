// Timer.c
// Header file for the Timer ADT
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#ifndef TIMER
#define TIMER

#include "Segment.h"

typedef struct timer *Timer;

typedef unsigned int uint;

Timer newTimer(uint gamma);

double startTimer(Timer timer);

double getTimeOutInterval(Timer timer);

uint getSampledSeqNo(Timer timer);

uint getSampledAckNo(Timer timer);

void startSamplingRTT(Timer timer, Segment s);

int isSamplingRTT(Timer timer);

void stopSamplingRTT(Timer timer);

void cancelSamplingRTT(Timer timer);

#endif

