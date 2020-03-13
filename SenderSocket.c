// SenderSocket.c
// Implementation of the SenderSocket ADT
// Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
// for the Simple Transport Protocol (COMP3331 18s2 Assignment)

#include <arpa/inet.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Segment.h"
#include "SenderSocket.h"

struct senderSocket {
	int                sockfd;
	struct sockaddr_in serveraddr;
	socklen_t          slen;
};

SenderSocket newSocket(char *recvIp, int recvPort) {

	SenderSocket ssock = calloc(1, sizeof(struct senderSocket));
	if (ssock == NULL) {
		errx(EXIT_FAILURE, "Insufficient memory!");
	}
	
	// Creating a UDP socket...
	if ((ssock->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		errx(EXIT_FAILURE, "Socket creation failed");
	}
	
	// Filling in the server information...
	(ssock->serveraddr).sin_family = AF_INET;
	(ssock->serveraddr).sin_port = htons(recvPort);
	if (!inet_aton(recvIp, &((ssock->serveraddr).sin_addr))) {
		errx(EXIT_FAILURE, "inet_aton() failed");
	}
	
	return ssock;
}

void sendSocket(SenderSocket ssock, int length, Segment s) {
	printf("Transmitting (sequence no. %d)\n", getSeqNo(s));
	ssock->slen = sizeof(ssock->serveraddr);
	if (sendto(ssock->sockfd, s, length + getHeaderSize(), 0,
			(struct sockaddr *) &(ssock->serveraddr),
			sizeof(ssock->serveraddr)) < 0) {
		errx(EXIT_FAILURE, "Failed to send segment");
	}
}

int socketGetReply(SenderSocket ssock, Segment s) {
	int recv_len;
	if ((recv_len = recvfrom(ssock->sockfd, s, 100, 0,
			(struct sockaddr *)  &(ssock->serveraddr),
			&(ssock->slen))) < 0) {
		errx(EXIT_FAILURE, "Failed to receive reply");
	}
	return recv_len;
}

void closeSocket(SenderSocket ssock) {
	close(ssock->sockfd);
}

