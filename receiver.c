/*
	sabertooth.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>
#include <fcntl.h>
#include <termios.h> // Serial settings
#include <sys/select.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <signal.h>
#include <time.h>

#include "intmath.h"
#include "sabertooth.h"
#include "net.h"
#include "control.h"
#include "timer.h"

int main (int argc, char * argv[]) {

	int rv;
	char buf[128];
	unsigned off;

	int socketfd = 0;
	struct sockaddr_in socketaddr;
	int socketread;
	
	struct packet pkt = {0, 0};

	int ttyfd = -1;
	unsigned uartspd = B9600;
	struct termios ttycfg;
	char ttydev[64] = "/dev/ttyUSB0";

	fd_set readfds;
	fd_set writefds;
	struct timeval tv= {0, 0};
	
	int8_t turn, speed;

	socketaddr.sin_family = AF_INET;
	socketaddr.sin_port = htons(9000);
	socketaddr.sin_addr.s_addr = 0;
	
	// Process optional arguments
	char optc;

	while ((optc = getopt (argc, argv, "p:t:s:")) != -1) {
    switch (optc)
		{
		case 'p':
			socketaddr.sin_port = htons(atoi(optarg));
			//strncpy(port, optarg, sizeof(port));
			//port[sizeof(port) - 1] = 0;
			break;
		case 't':
			strncpy(ttydev, optarg, sizeof(ttydev));
			ttydev[sizeof(ttydev) - 1] = 0;
			break;
		case 's':
			// (*@&$@ baud rates are enumerated...
			switch (atoi(optarg)) {
				case 9600:
					uartspd = B9600;
					break;
				case 19200:
					uartspd = B19200;
					break;
				case 38400:
					uartspd = B38400;
					break;
				default:
					fprintf(stderr, "Baud rate not supported\n");
					exit(1);
			}
			break;
		default:
			exit(1);
		}
	}
	
	// Open our output tty
	if ((ttyfd = open(ttydev, O_RDWR | O_NOCTTY | O_NDELAY)) < 0) {
		fprintf(stderr, "Can't open device %s: %s\n", ttydev, strerror(errno));
		exit(1);
	}
	if(!isatty(ttyfd)) {
		fprintf(stderr, "Can't open device %s as tty: %s\n", ttydev, strerror(errno));
		exit(1);
	}
	// Set UART speed
	if(tcgetattr(ttyfd, &ttycfg) < 0) { 
		fprintf(stderr, "Can't get attributes for device %s: %s\n", ttydev, strerror(errno));
		exit(1);
	}
	cfsetospeed(&ttycfg,uartspd);
	cfsetispeed(&ttycfg,uartspd);
	cfmakeraw(&ttycfg);
	if(tcsetattr(ttyfd,TCSAFLUSH,&ttycfg) < 0) {
		fprintf(stderr, "Can't set attributes for device \"%s\": %s\n", ttydev, strerror(errno));
		exit(1);
	}

	// Gett a listening datagram socket
	if ((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		fprintf(stderr, "Socket error: %s\n", strerror(errno));
		exit(1);
	}

	if (bind(socketfd, (struct sockaddr *) &socketaddr, sizeof(socketaddr)) == -1) {
		fprintf(stderr, "Bind error: %s\n", strerror(errno));
		exit(1);
	}

	// Print status
	fprintf(stderr, "Listen port: %d, Output tty: %s, Serial speed: %u\n", ntohs(socketaddr.sin_port), ttydev, cfgetospeed(&ttycfg));
	
	// Set up timer
	if (timer_init() < 0) exit(1);
	if (timer_start() < 0) exit(1);
	
	// Main event loop begins here
	for(;;) {
		FD_ZERO(&readfds);
		FD_SET(socketfd, &readfds);
	
		FD_ZERO(&writefds);
		FD_SET(ttyfd, &writefds);
		
		rv = select(FD_SETSIZE, &readfds, &writefds, NULL, &tv);
		
		if (rv >= 0) {
			if (FD_ISSET(socketfd, &readfds)) {
				if((rv = recvfrom(socketfd, buf, sizeof(buf), 0, NULL, 0)) == -1  && errno != EINTR) {
					fprintf(stderr, "Failed to read from socket: %s\n", strerror(errno));
					exit(1);
				}
				unpack(buf, &pkt.command, &pkt.value);
				switch (pkt.command) {
					case 0:
						j5setspeed(pkt.value);
						break;
					case 1:
						j5setturn(pkt.value);
						break;
					default:
						break;
				}
			}
			if (FD_ISSET(ttyfd, &writefds)) {
				turn = (int8_t) iadd(j5getturn() >> 23, 0, -127, 127);
				speed = (int8_t) iadd(j5getspeed() >> 23, 0, -127, 127);
				
				off = 0;
				// Synchronization byte
				buf[off++] = ST_START;
				
				if (speed >= 0) off += st_command(&buf[off], 0, ST_DRV_FWD, speed);
				if (speed < 0) off += st_command(&buf[off], 0, ST_DRV_REV, -speed);
				if (turn >= 0) off += st_command(&buf[off], 0, ST_TRN_RHT, turn);
				if (turn < 0) off += st_command(&buf[off], 0, ST_TRN_LFT, -turn);
				
				rv = write(ttyfd, &buf, off);
				if (rv == -1) {
					switch (errno) {
						case EINTR:
							// This only gets set if no data was written, so it's safe to ignore it.
							break;
						case EAGAIN:
						//case EWOULDBLOCK:
							// We don't actually handle these, which may not necessarily be a problem...
							break;
						default:
							fprintf(stderr, "Write error: %s\n", strerror(errno)); 
							exit(1);
					}
				}
			}
			if (!tv.tv_sec) {
				tv.tv_usec = 200000; // Reset timeout
				fprintf(stderr, "Speed %-4d Turn %-4d\r", speed, turn);
			}
		}
		else {
			if (errno != EINTR) {
				fprintf(stderr, "select() error: %s\n", strerror(errno));
				exit(1);
			}
		}
	}
}
