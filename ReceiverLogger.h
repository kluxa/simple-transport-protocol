// ReceiverLogger.h
// Header file for the ReceiverLogger ADT
// The ReceiverLogger ADT logs events and maintain statistics
// for the receiver
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#ifndef RECEIVER_LOGGER
#define RECEIVER_LOGGER

#include "Segment.h"

typedef struct receiverLogger *ReceiverLogger;

typedef unsigned int Event;

#define SENT           (1U <<  0)
#define RECEIVED       (1U <<  1)
#define DROPPED        (1U <<  2)                     // Sender only
#define CORRUPTED      (1U <<  3)                     // Sender only
#define DUPLICATED     (1U <<  4)                     // Sender only
#define REORDERED      (1U <<  5)                     // Sender only
#define DELAYED        (1U <<  6)                     // Sender only
#define DUPLICATE_ACK  (1U <<  7)
#define TIMEOUT_REXMIT (1U <<  8)                     // Sender only
#define FAST_REXMIT    (1U <<  9)                     // Sender only
#define RETRANSMITTED  (TIMEOUT_REXMIT | FAST_REXMIT) // Sender only
#define DUPLICATE_DATA (1U << 10)                     // Receiver only
#define CORRUPTED_DATA (1U << 11)                     // Receiver only

ReceiverLogger newReceiverLogger(char *filename);

void logEvent(ReceiverLogger logger, Event e, Segment s);

void logSummary(ReceiverLogger logger);

#endif

