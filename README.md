# skids
This is motor control software for the PEMS Autonomous Car project.  It is written in C for Linux, but may be able to compile on other POSIX-compliant systems.
The software consists of three programs: controller, receiver, and jcmd.  Controller and receiver communicate via UDP packets sent over a network.

controller - Attach to an input device and send X/Y axis data to receiver.

receiver - Listen for data packets, update the speed/turn set points, and write commands to a serial device.

jcmd - Create a command packet and write it to stdout.  This is nice for testing.

Siempre aprendiendo.
