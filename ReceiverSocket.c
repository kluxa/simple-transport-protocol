// ReceiverSocket.c
// Implementation of the ReceiverSocket ADT
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#include <arpa/inet.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ReceiverSocket.h"

struct receiverSocket {
	int                sockfd;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	socklen_t          slen;
};

ReceiverSocket newSocket(int recvPort) {
	ReceiverSocket rsock = calloc(1, sizeof(struct receiverSocket));
	if (rsock == NULL) {
		errx(EXIT_FAILURE, "Insufficient memory!");
	}

	// Creating a UDP socket...
	if ((rsock->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		errx(EXIT_FAILURE, "Socket creation failed");
	}
	
	// Filling in server information...
	(rsock->serveraddr).sin_family = AF_INET;
	(rsock->serveraddr).sin_addr.s_addr = INADDR_ANY;
	(rsock->serveraddr).sin_port = htons(recvPort);
	
	// Bind the socket to the server address...
	if (bind(rsock->sockfd, (struct sockaddr *)&(rsock->serveraddr),
			sizeof(rsock->serveraddr)) < 0) {
		errx(EXIT_FAILURE, "Bind failed");
	}
	
	rsock->slen = sizeof(rsock->clientaddr);
	return rsock;
}

int receiveSocket(ReceiverSocket rsock, int length, Segment s) {
	int recv_len;
	if ((recv_len = recvfrom(rsock->sockfd, s, length, 0,
			(struct sockaddr *)&(rsock->clientaddr),
			&rsock->slen)) < 0) {
		errx(EXIT_FAILURE, "Failed to receive data");
	}
	
	return recv_len;
}

void replySocket(ReceiverSocket rsock, int length, Segment s) {
	if (sendto(rsock->sockfd, s, length + getHeaderSize(), 0,
			(struct sockaddr *)(&(rsock->clientaddr)),
			rsock->slen) < 0) {
		errx(EXIT_FAILURE, "Failed to send a reply");
	}
}

void closeSocket(ReceiverSocket rsock) {
	close(rsock->sockfd);
}

