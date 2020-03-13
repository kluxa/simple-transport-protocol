# Makefile
# Controls compilation for the sender and receiver
# Written by Kevin Luxa (z5074984 - k.luxa@student.unsw.edu.au)
# for the Simple Transport Protocol (COMP3331 18s2 Assignment)

CC = gcc
CFLAGS = -Wall -Werror -std=gnu99

all: sender receiver

SEND_OBJS = sender.o SenderSTP.o SenderSocket.o SenderLogger.o SenderWindow.o SenderPLD.o Timer.o Segment.o Queue.o
RECV_OBJS = receiver.o ReceiverSTP.o ReceiverSocket.o ReceiverLogger.o Segment.o Queue.o

sender: $(SEND_OBJS)
	$(CC) $(CFLAGS) -o sender -pthread $(SEND_OBJS)

receiver: $(RECV_OBJS)
	$(CC) $(CFLAGS) -o receiver -pthread $(RECV_OBJS)

sender.o: sender.c
SenderSTP.o: SenderSTP.c
SenderLogger.o: SenderLogger.c
SenderSocket.o: SenderSocket.c
SenderWindow.o: SenderWindow.c
SenderPLD.o: SenderPLD.c
Timer.o: Timer.c

receiver.o: receiver.c
ReceiverSTP.o: ReceiverSTP.c
ReceiverLogger.o: ReceiverLogger.c
ReceiverSocket.o: ReceiverSocket.c

Segment.o: Segment.c
Queue.o: Queue.c

clean:
	rm -f sender receiver *.o

