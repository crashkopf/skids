/*
	sabertooth.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "libevdev/libevdev.h"
#include "net.h"

int main (int argc, char * argv[]) {
	int status;
	
	int inputfd;
	char inputdev[64] = "/dev/input/event0";
	struct libevdev *input = NULL;
	
	int socketfd;
	char host[64] = "127.0.0.1";
	char port[6] = "9000";
	int socksent;
	
	struct packet pkt;
	unsigned char pbuf[32];
	
	struct input_event ievent;
	int Xmin,Xmax,Ymin,Ymax;

	//Process optional arguments
	char optc;
	
	while ((optc = getopt (argc, argv, "i:h:p:")) != -1) {
    switch (optc)
		{
		case 'i':
			strncpy(inputdev, optarg, sizeof(inputdev));
			inputdev[sizeof(inputdev) - 1] = 0;
			break;
		case 'h':
			strncpy(host, optarg, sizeof(host));
			host[sizeof(host) - 1] = 0;
			break;
		case 'p':
			strncpy(port, optarg, sizeof(port));
			port[sizeof(port) - 1] = 0;
			break;
		default:
			exit(1);
		}
	}
	
	// Open our input device
	if ((inputfd = open(inputdev, O_RDONLY)) < 0) {
		fprintf(stderr, "Failed to open device %s: %s\n", inputdev, strerror(errno));
		exit(1);
	}

	if ((status = libevdev_new_from_fd(inputfd, &input)) < 0) {
		fprintf(stderr, "Failed to init libevdev: %s\n", strerror(-status));
        exit(1);
	}
	if (!libevdev_has_event_type(input, EV_ABS)) {
		fprintf(stderr, "Input device not supported\n");
        exit(1);
	}
	if (libevdev_grab(input, LIBEVDEV_GRAB)) {
		fprintf(stderr, "Failed to grab input device\n");
	}
	else {
		fprintf(stderr, "Grabbed input device\n");
	}
	
	fprintf(stderr, "Opened input device %s\n", inputdev);
	
	// Get ourselves a socket
	socketfd = get_socket(host, port);
	
	fprintf(stderr, "Opened socket to %s:%s\n", host, port);

	// Main event loop begins here
	for(;;) {
		status = libevdev_next_event(input, LIBEVDEV_READ_FLAG_NORMAL, &ievent);
		switch (status) {
			case 0:
				pack(pbuf, ievent.code, ievent.value);
				//fprintf(stderr, "%u:%li", pkt.command, pkt.value);
				socksent = send(socketfd, pbuf, 6, 0);
				break;
			case 1:
				// SYN dropped
				break;
			case -EAGAIN:
				break;
			default:
				fprintf(stderr, "Read error: %s\n", strerror(errno));
				exit(1);
		}
	}
	// on_exit() ...
	//libevdev_grab(input, 0)
	//close(inputfd);
	//close(socketfd);
}
