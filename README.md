# simple-transport-protocol
Implementation of a simple version of TCP over UDP with a packet error simulator.


## Features
The sender and receiver programs implement these features:
- Three-way handshake for connection establishment
- Four-segment connection termination
- Duplicate packet detection
- Single bit error detection (using a parity bit)
- Fast retransmit (retransmit immediately upon 3 duplicate ACKs)
- Pipelining
- Timer for round-trip-time estimation
- In-order delivery to the application layer
- Simulation of errors, including:
  - Packet drops
  - Packet duplication
  - Single bit corruption
  - Packet reordering
  - Packet delay
