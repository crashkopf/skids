/*
	sabertooth.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
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
	int lastX, lastY;
	int minX,maxX,minY,maxY;
	int centerX, centerY;
	double scaleX, scaleY;

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
	//if ((inputfd = open(inputdev, O_RDONLY|O_NONBLOCK)) < 0) {
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
	// Try to grab it
	if (libevdev_grab(input, LIBEVDEV_GRAB)) {
		fprintf(stderr, "Failed to grab input device\n");
	}
	else {
		fprintf(stderr, "Grabbed input device\n");
	}
	
	fprintf(stderr, "Opened input device %s\n", inputdev);
	
	// Figure out how to zero the axes
	minX = libevdev_get_abs_minimum(input, ABS_X);
	maxX = libevdev_get_abs_maximum(input, ABS_X);
	minY = libevdev_get_abs_minimum(input, ABS_Y);
	maxY = libevdev_get_abs_maximum(input, ABS_Y);
	
	centerX = ((maxX - minX) / 2) + minX;
	centerY = ((maxY - minY) / 2) + minY;
	
	scaleX = ((double) INT_MAX) / ((double) (maxX - minX));
	scaleY = ((double) INT_MAX) / ((double) (maxY - minY));
	
	libevdev_fetch_event_value(input, EV_ABS, ABS_X, &lastX);
	lastX = (lastX - centerX) * scaleX;
	libevdev_fetch_event_value(input, EV_ABS, ABS_Y, &lastY);
	lastY = (lastY - centerY) * scaleY;
	
	fprintf(stderr, "X axis - min %d, max %d, range %d, center %d, scale %f\n", minX, maxX, (maxX - minX), centerX, scaleX);
	fprintf(stderr, "Y axis - min %d, max %d, range %d, center %d, scale %f\n", minY, maxY, (maxY - minY), centerY, scaleY);
	
	// Get ourselves a socket
	socketfd = get_socket(host, port);
	
	fprintf(stderr, "Opened socket to %s:%s\n", host, port);

	fprintf(stderr, "X: %-012d Y: %-012d\r", lastX, lastY);

	// Main event loop begins here
	for(;;) {
		status = libevdev_next_event(input, LIBEVDEV_READ_FLAG_NORMAL|LIBEVDEV_READ_FLAG_BLOCKING, &ievent);
		switch (status) {
			case LIBEVDEV_READ_STATUS_SUCCESS:
				if (ievent.type == EV_ABS) {
					switch (ievent.code) {
						case ABS_Y:
							lastY = (ievent.value - centerY) * scaleY;
							break;
						case ABS_X:
							lastX = (ievent.value - centerX) * scaleX;
							break;
						default:
							break;
					}
				}
				fprintf(stderr, "X: %-12d Y: %-12d\r", lastX, lastY);
				
				pack(pbuf, 0, lastY);
				socksent = send(socketfd, pbuf, 6, 0);
				pack(pbuf, 1, lastX);
				socksent = send(socketfd, pbuf, 6, 0);
				break;
			case LIBEVDEV_READ_STATUS_SYNC:
				// SYN dropped
				fprintf(stderr, "SYN_DROPPED!");
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
