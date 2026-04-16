
Hi there, ty for opening this file ;)
This repository contains a C implementation of a basic Reliable File Transfer Protocol designed to transfer generic byte streams over UDP.

Project Scope & Limitations:
Please note that this is a hobbyist-level project intended for educational purposes (e.g., LAN environments or local testing). While it demonstrates the core principles of a reliable UDP protocol, it should be adapted for production use.
Current limitations include:
-Packet Loss: Simulated via software logic.
-Timer Accuracy: Functional but not high-precision.
-Serialization: Data is transferred via packed structs; for cross-architecture usage, a formal serialization layer should be added.
-Edge Cases: Certain packet loss scenarios during specific handshake phases may require additional robust error checking.

Environment and what to use:
OS: Designed for UNIX-like systems (Tested on Ubuntu and Fedora).
Compiler: Any C compiler (GCC is recommended if a chose as to be made).
Privileges: No root privileges required.

What it does?
The system mimics TCP-like behaviors using:
Selective Repeat ARQ: For efficient packet recovery.
Adaptive Retransmission Timeouts: To handle network jitter.
Hybrid Control Plane: A TCP connection manages the initial session establishment, while the R-UDP plane handles the actual data integrity.

How to Run
Server: ./server [Port] [Window Size] [Loss Prob %] [Timer Duration] [fisso/adattivo]
Client: ./client [Server IP] [Port] [Window Size] [Loss Prob %] [Timer Duration] [fisso/adattivo]
