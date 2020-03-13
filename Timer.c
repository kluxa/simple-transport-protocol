// Timer.c
// Implementation of the Timer ADT
// Maintains the RTO, calculates the sample RTT and updates
// the RTO interval
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#include <err.h>
#include <math.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "Timer.h"

typedef unsigned int uint;

struct timer {
	uint   gamma;
	double timeOutInterval;
	double estimatedRTT;
	double devRTT;
	
	struct timeval start;
	uint           sampledSeqNo; // Sequence no. of the segment we're using
	                             // to take a sample RTT.
	uint           sampledAckNo; // Acknowledgement no. of the segment we're
	                             // expecting back.
	uint           isSampling;   // Indicates whether we are timing the RTT
	                             // of a segment at the moment (or not)
	sem_t          lock;
};

static void updateTimeOutInterval(Timer timer, double sampleRTT);
static double timeDiff(struct timeval t0, struct timeval t1);

static void wakeUp(int sigID) {
	// Do nothing
}

Timer newTimer(uint gamma) {
	Timer timer = malloc(sizeof(struct timer));
	if (timer == NULL) {
		errx(EXIT_FAILURE, "Insufficient memory! (newTimer)");
	}
	
	timer->gamma = gamma;
	timer->estimatedRTT = 0.50;
	timer->devRTT = 0.25;
	timer->timeOutInterval = timer->estimatedRTT + timer->gamma * timer->devRTT;
	
	timer->isSampling = 0;
	sem_init(&(timer->lock), 0, 1);
	
	if (signal(SIGALRM, wakeUp) == SIG_ERR) {
		errx(EXIT_FAILURE, "Can't set signal handler\n");
	}
	
	return timer;
}

double startTimer(Timer timer) {
	int intPart = (int)(timer->timeOutInterval);
	int decPart = 1000000000 * (timer->timeOutInterval - intPart);
	struct timespec duration = {intPart, decPart};
	struct timespec remaining;
	
	int e = nanosleep(&duration, &remaining);
	double timeRemaining = (e == 0) ? 0 : (remaining.tv_sec * 1.0 +
	                       remaining.tv_nsec / 1000000000.0);
	
	return timeRemaining;
}

double getTimeOutInterval(Timer timer) {
	return timer->timeOutInterval;
}

uint getSampledSeqNo(Timer timer) {
	return timer->sampledSeqNo;
}

uint getSampledAckNo(Timer timer) {
	return timer->sampledAckNo;
}

void startSamplingRTT(Timer timer, Segment s) {
	sem_wait(&(timer->lock));
	timer->sampledSeqNo = getSeqNo(s);
	timer->sampledAckNo = getSeqNo(s) + getDataLength(s);
	gettimeofday(&(timer->start), NULL);
	timer->isSampling = 1;
	sem_post(&(timer->lock));
}

int isSamplingRTT(Timer timer) {
	int result;
	sem_wait(&(timer->lock));
	result = timer->isSampling;
	sem_post(&(timer->lock));
	return result;
}

void stopSamplingRTT(Timer timer) {
	sem_wait(&(timer->lock));
	struct timeval now;
	gettimeofday(&now, NULL);
	double sampleRTT = timeDiff(timer->start, now);
	updateTimeOutInterval(timer, sampleRTT);
	timer->isSampling = 0;
	sem_post(&(timer->lock));
}

static void updateTimeOutInterval(Timer timer, double sampleRTT) {
	timer->estimatedRTT = (0.875 * timer->estimatedRTT) + (0.125 * sampleRTT);
	timer->devRTT = (0.75 * timer->devRTT) + (0.25 * fabs(sampleRTT - timer->estimatedRTT));
	timer->timeOutInterval = timer->estimatedRTT + timer->gamma * timer->devRTT;
	
	if (timer->timeOutInterval < 0.1)
		timer->timeOutInterval = 0.1;
	
	// These lines were enabled for
	// parts b and c of the report.
	//if (timer->timeOutInterval > 1.0)
	//	timer->timeOutInterval = 1.0;
}

void cancelSamplingRTT(Timer timer) {
	sem_wait(&(timer->lock));
	timer->isSampling = 0;
	sem_post(&(timer->lock));
}

static double timeDiff(struct timeval t0, struct timeval t1) {
	return (t1.tv_usec - t0.tv_usec) / 1000000.0f +
	       (t1.tv_sec - t0.tv_sec);
}

