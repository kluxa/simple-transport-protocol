// sender.c
// Application layer for the Simple Transport Protocol (STP)
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

// To run: ./sender <receiver_host_ip> <receiver_port> <file> <MWS> <MSS> <gamma> <pDrop> <pDuplicate> <pCorrupt> <pOrder> <maxOrder> <pDelay> <maxDelay> <seed>
// Example: ./sender 127.0.0.1 1834 files/test0.pdf 1000 100 6 0 0 0 0 0 0 0 0

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "SenderSTP.h"

typedef unsigned int uint;

char *RECEIVER_IP;
uint  RECEIVER_PORT;
char *FILENAME;
uint  MWS;
uint  MSS;
uint  GAMMA;
float P_DROP;
float P_DUPLICATE;
float P_CORRUPT;
float P_ORDER;
uint  MAX_ORDER;
float P_DELAY;
uint  MAX_DELAY;
uint  SEED;

void checkArgs(int argc, char *argv[]);
void setArgs(char *argv[]);

int main(int argc, char *argv[]) {
	setbuf(stdout, NULL);
	checkArgs(argc, argv);
	setArgs(argv);
	srand(SEED);
	
	SenderSTP sstp = newSTP(RECEIVER_IP, RECEIVER_PORT, MWS, MSS, GAMMA,
	                        P_DROP, P_DUPLICATE, P_CORRUPT, P_ORDER,
	                        MAX_ORDER, P_DELAY, MAX_DELAY);
	
	////////////////////////////////////////////////////////////////////
	// Establishment
	establishSTP(sstp);
	printf("Connection established.\n");
	
	////////////////////////////////////////////////////////////////////
	// File Transfer
	char data[MSS];
	
	int fd = open(FILENAME, O_RDONLY);
	if (fd == -1) {
		errx(EXIT_FAILURE, "Couldn't open %s", FILENAME);
	}
	
	int nbytes;
	while ((nbytes = read(fd, data, MSS)) > 0) {
		pushDataToSTP(sstp, nbytes, data);
	}
	
	close(fd);
	if (nbytes < 0) {
		errx(EXIT_FAILURE, "Read failed");
	}
	
	////////////////////////////////////////////////////////////////////
	// Teardown
	teardownSTP(sstp);
	printf("Connection terminated.\n");
	
	return 0;
}

////////////////////////////////////////////////////////////////////////

void checkArgs(int argc, char *argv[]) {
	char *progname = argv[0];
	if (argc != 15)
		errx(EXIT_FAILURE, "Usage: %s <ip> <port> <file> <MWS> <MSS> <gamma> <pDrop> <pDuplicate> <pCorrupt> <pOrder> <maxOrder> <pDelay> <maxDelay> <seed>", progname);
	if (atoi(argv[2]) <= 1024)
		errx(EXIT_FAILURE, "%s: port should be an integer greater than 1024", progname);
	struct stat buffer;
	if (stat(argv[3], &buffer) != 0)
		errx(EXIT_FAILURE, "%s: the file %s doesn't exist", progname, argv[3]);
	if (atoi(argv[4]) <= 0)
		errx(EXIT_FAILURE, "%s: MWS should be a positive integer", progname);
	if (atoi(argv[5]) <= 0)
		errx(EXIT_FAILURE, "%s: MSS should be a positive integer", progname);
}

void setArgs(char *argv[]) {
	RECEIVER_IP   =      argv[ 1];
	RECEIVER_PORT = atoi(argv[ 2]);
	FILENAME      =      argv[ 3];
	MWS           = atoi(argv[ 4]);
	MSS           = atoi(argv[ 5]);
	GAMMA         = atoi(argv[ 6]);
	P_DROP        = atof(argv[ 7]);
	P_DUPLICATE   = atof(argv[ 8]);
	P_CORRUPT     = atof(argv[ 9]);
	P_ORDER       = atof(argv[10]);
	MAX_ORDER     = atoi(argv[11]);
	P_DELAY       = atof(argv[12]);
	MAX_DELAY     = atoi(argv[13]);
	SEED          = atoi(argv[14]);
}

