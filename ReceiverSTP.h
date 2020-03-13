// ReceiverSTP.c
// Header file for the ReceiverSTP ADT

// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for COMP3331 18s2 Assignment

#include "ReceiverSocket.h"

typedef struct receiverSTP *ReceiverSTP;

ReceiverSTP newSTP(int recvPort);

int pullDataFromSTP(ReceiverSTP rstp, char **data);

void establishSTP(ReceiverSTP rstp);

void teardownSTP(ReceiverSTP rstp);

