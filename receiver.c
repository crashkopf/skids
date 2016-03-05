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
#include <termios.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "intmath.h"
#include "sabertooth.h"
#include "net.h"

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
	
	int socketfd = 0;
	char port[6] = "9000";
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;
	int socketread;
    char socketbuf[128];
	
	struct packet pkt;
	
	int ttyfd = -1;
	unsigned uartspd = 38400;
	struct termios ttycfg;
	char ttydev[64] = "/dev/ttyS0";
	
	int32_t speed = 0; // positive is Forward
	int32_t turn = 0;  // positive is to the right
	
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
			//uartspd = atoi(optarg);
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
	cfsetospeed(&ttycfg,B38400);
	cfsetispeed(&ttycfg,B38400);
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
	fprintf(stderr, "Listen port: %s, Output tty: %s, Serial speed: %u\n", port, ttydev, uartspd);
	
	// Main event loop begins here

	fd_set readfds;
	fd_set writefds;
	
	int ready;

	struct timeval tv;
	while (!done) {
		FD_ZERO(&readfds);
		FD_SET(socketfd, &readfds);
	
		FD_ZERO(&writefds);
		FD_SET(ttyfd, &writefds);
		
		tv.tv_sec  = 0;
		tv.tv_usec = 100;
		
		ready = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);
		switch(ready) {
			case -1:
				perror("select");
				done = 1;
			break;
			case 0:
			default:
				break;
		}

		if (FD_ISSET(socketfd, &readfds)) {
			socketread = recvfrom(socketfd, socketbuf, sizeof(socketbuf), 0, NULL, NULL);
			if (socketread < 0) {
				fprintf(stderr, "Failed to read from socket: %s\n", strerror(errno));
			}
			unpack(socketbuf, &pkt.command, &pkt.value);
			fprintf(stderr, "(%d) %04X:%08lX\n", socketread, pkt.command, pkt.value);
			
			fprintf(stderr, "Received packet %d bytes long\n", socketread);
			for (int i = 0; i < socketread; i++) {
				fprintf(stderr, " %02hhX", socketbuf[i]);
			}
			fputs("\n", stderr);
			for (int i = 0; i < socketread; i++) {
				if (isprint(socketbuf[i]))
					fprintf(stderr, " % 2c", socketbuf[i]);
				else 
					fputs(" ..", stderr);
			}
			fputs("\n\n", stderr);
			
		}
		
		if (FD_ISSET(ttyfd, &writefds)) {
			
		}
		
		
	}
	// Clean-up code


}