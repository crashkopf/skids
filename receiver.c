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
#include <netdb.h> // gethostaddr()
#include <arpa/inet.h> 

#include <signal.h>
#include <time.h>

#include "intmath.h"
#include "sabertooth.h"
#include "net.h"
#include "control.h"
#include "timer.h"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main (int argc, char * argv[]) {

	int done = 0;
	int status;
	int ready;
	int rv;

	int socketfd = 0;
	char port[6] = "9000";
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;
	int socketread;
	char socketbuf[128];

	struct packet pkt = {0, 0};

	int ttyfd = -1;
	unsigned uartspd = B9600;
	struct termios ttycfg;
	char ttydev[64] = "/dev/ttyUSB0";

	st_packet stp;
	int8_t st = ST_START;

	fd_set readfds;
	fd_set writefds;
	struct timeval tv= {0, 0};
	
	int8_t turn, speed;

	// Process optional arguments
	char optc;

	while ((optc = getopt (argc, argv, "p:t:s:")) != -1) {
    switch (optc)
		{
		case 'p':
			strncpy(port, optarg, sizeof(port));
			port[sizeof(port) - 1] = 0;
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

	// Open listening socket (shamelessly stolen from Beej's network programming guide
	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_DGRAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo() error: %s\n", gai_strerror(status));
		exit(1);
	}
	// loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((socketfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            fprintf(stderr, "Socket error: %s\n", strerror(errno));
            continue;
        }

        if (bind(socketfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(socketfd);
            fprintf(stderr, "Bind error: %s\n", strerror(errno));
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Failed to bind socket\n");
        exit(2);
    }

    freeaddrinfo(servinfo);

	// Print status
	fprintf(stderr, "Listen port: %s, Output tty: %s, Serial speed: %u\n", port, ttydev, cfgetospeed(&ttycfg));
	
	// Set up timer
	if (timer_init() < 0) exit(1);
	if (timer_start() < 0) exit(1);
	
	// Main event loop begins here
	while (!done) {
		FD_ZERO(&readfds);
		FD_SET(socketfd, &readfds);
	
		FD_ZERO(&writefds);
		FD_SET(ttyfd, &writefds);
		
		rv = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);
		
		if (rv >= 0) {
			if (FD_ISSET(socketfd, &readfds)) {
				if((rv = recvfrom(socketfd, socketbuf, sizeof(socketbuf), 0, NULL, 0)) == -1  && errno != EINTR) {
					fprintf(stderr, "Failed to read from socket: %s\n", strerror(errno));
					exit(1);
				}
				unpack(socketbuf, &pkt.command, &pkt.value);
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
				
				// Send synchronization byte
				if ((write(ttyfd, &st, 1) == -1) && errno != EINTR) {fprintf(stderr, "Write sync error: %s\n", strerror(errno)); exit(1);}
				
				if (speed > 0) {
					stp = st_command(0, ST_DRV_FWD, speed);
					if ((write(ttyfd, &stp, 4) == -1) && errno != EINTR) {fprintf(stderr, "Write error: %s\n", strerror(errno)); exit(1);}
				}
				if (speed < 0) {
					stp = st_command(0, ST_DRV_REV, -speed);
					if ((write(ttyfd, &stp, 4) == -1) && errno != EINTR) {fprintf(stderr, "Write error: %s\n", strerror(errno)); exit(1);}
				}
				if (turn > 0) {
					stp = st_command(0, ST_TRN_RHT, turn);
					if ((write(ttyfd, &stp, 4) == -1) && errno != EINTR) {fprintf(stderr, "Write error: %s\n", strerror(errno)); exit(1);}
				}
				if (turn < 0) {
					stp = st_command(0, ST_TRN_LFT, -turn);
					if ((write(ttyfd, &stp, 4) == -1) && errno != EINTR) {fprintf(stderr, "Write error: %s\n", strerror(errno)); exit(1);}
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
