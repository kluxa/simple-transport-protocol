// receiver.c
// Application layer for the Simple Transport Protocol (STP)
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

// To run: ./receiver <port> <new filename>
// Example: ./receiver 1834 new_file.pdf

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ReceiverSTP.h"

void checkArgs(int argc, char *argv[]);
void setArgs(char *argv[]);

int   RECEIVER_PORT;
char *NEW_FILENAME;

int main(int argc, char *argv[]) {
	// setbuf(stdout, NULL);
	checkArgs(argc, argv);
	setArgs(argv);
	
	ReceiverSTP rstp = newSTP(RECEIVER_PORT);
	
	////////////////////////////////////////////////////////////////////
	// Establishment
	establishSTP(rstp);
	printf("Connection established.\n");
	
	////////////////////////////////////////////////////////////////////
	// File Transfer
	char *data;
	int nbytes;
	
	int fd = open(NEW_FILENAME, O_CREAT|O_RDWR|O_TRUNC);
	while (1) {
		nbytes = pullDataFromSTP(rstp, &data);
		if (nbytes == 0) break;
		write(fd, data, nbytes);
		free(data);
	}
	close(fd);
	
	printf("About to teardown connection\n");
	
	////////////////////////////////////////////////////////////////////
	// Teardown
	teardownSTP(rstp);
	printf("Connection terminated.\n");
	
	return 0;
}

////////////////////////////////////////////////////////////////////////

void checkArgs(int argc, char *argv[]) {
	if (argc != 3)
		errx(EXIT_FAILURE, "Usage: %s <port> <new filename>", argv[0]);
	if (atoi(argv[1]) <= 1024)
		errx(EXIT_FAILURE, "port should be an integer greater than 1024");
}

void setArgs(char *argv[]) {
	RECEIVER_PORT = atoi(argv[1]);
	NEW_FILENAME  =      argv[2];
}

