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

//#include <linux/input.h>
#include "input/input.h"
#include "intmath.h"
#include "sabertooth.h"

int main (int argc, char * argv[]) {

	int done = 0;
	
	int inputfd = 0;
	int outputfd = 1;
	
	unsigned uartspd = 9600;
	
	int32_t speed = 0; // positive is Forward
	int32_t turn = 0;  // positive is to the right
	
	int Xmin,Xmax,Ymin,Ymax;

	char definput[] = "/dev/input/event0";

/*
	Process optional arguments
*/
	char * ifarg = 0;
	char * ofarg = 0;
	char * spdarg = 0;
	
	char optc;
	
	while ((optc = getopt (argc, argv, "i:o:s:")) != -1) {
    switch (optc)
		{
		case 'i':
			ifarg = optarg;
			break;
		case 'o':
			ofarg = optarg;
			break;
		case 's':
			spdarg = optarg;
			break;
		default:
			abort ();
		}
	}
	
	fprintf(stderr, "Input: %s(%i), output %s(%i), serial speed %u\n", ifarg, inputfd, ofarg, outputfd, uartspd);
	
	// Open our input device
	if (!ifarg) ifarg = definput;
	if (-1 == (inputfd = open(ifarg, O_RDONLY))) {
		fprintf(stderr, "Failed to open %s: %s\n", ifarg, strerror(errno));
		done = 1;
	}
	
	device_info(-1, inputfd, 1);
	
	// Check for absolute position event support
	evtmap evtypes;
	if (ioctl (inputfd, EVIOCGBIT (0, sizeof (evtypes)), evtypes) < 0) {
		perror ("EVIOCGBIT ioctl failed");
		done = 1;
    }
	if (!test_bit(EV_ABS, evtypes)) {
		perror("ABS events not supported");
		done = 1;
	}
	
	// Get info on our axes
	struct input_absinfo absinfo;
	if (ioctl (inputfd, EVIOCGABS(ABS_X), &absinfo) < 0) {
		perror ("EVIOCGABS(ABS_X) ioctl failed");
		done = 1;
    }
	else {
		Xmin = absinfo.minimum;
		Xmax = absinfo.maximum;
	}
	if (ioctl (inputfd, EVIOCGABS(ABS_Y), &absinfo) < 0) {
		perror ("EVIOCGABS(ABS_Y) ioctl failed");
		done = 1;
    }
	else {
		Ymin = absinfo.minimum;
		Ymax = absinfo.maximum;
	}
	
	fprintf(stderr, "Xmin: %d Xmax: %d Ymin: %d Ymax: %d\n", Xmin, Xmax, Ymin, Ymax);
	
	// Grab it so nobody else is getting events
	if (-1 == ioctl(inputfd,EVIOCGRAB,1)) {
			perror("EVIOCGRAB(1) ioctl failed");
			close(inputfd);
			done = 1;
		}
		fprintf(stderr,"Grabbed input device\n");
	
	if (spdarg) uartspd = atoi(spdarg);
	
	
	fprintf(stderr, "Speed % 4i Turn % 4i\r", speed, turn);
	
/*
	Main event loop begins here
*/
	fd_set readfds;
	fd_set writefds;
	
	int ready;
	
	struct input_event ievent;
	struct timeval tv;
	while (!done) {
		FD_ZERO(&readfds);
		FD_SET(inputfd, &readfds);
	
		FD_ZERO(&writefds);
		FD_SET(STDOUT_FILENO, &writefds);
		
		tv.tv_sec  = 0;
		tv.tv_usec = 100;
		
		ready = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);
		switch(ready) {
			case -1:
				perror("select");
				done = 1;
			break;
			case 0:
				//~ if (speed > 0) speed--;
				//~ if (speed < 0) speed++;
				//~ if (turn > 0) turn--;
				//~ if (turn < 0) turn++;
				
				//~ break;
			default:
				fprintf(stderr, "Speed % 4i Turn % 4i\r", speed, turn);
				//fprintf(stderr, "Ready! (%i)\n", ready);
				break;
		}

		if (FD_ISSET(inputfd, &readfds)) {
			switch (read(inputfd,&ievent,sizeof(ievent))) {
				case -1:
					perror("read");
					done = 1;
					break;
				case 0:
					fprintf(stderr,"EOF\n");
					done = 1;
					break;
				default:
					if (ievent.type == EV_ABS) {
						if (ievent.code == ABS_X) {
							turn = (ievent.value - ((Xmax - Xmin) / 2 )) * 255/(Xmax - Xmin);
						}
						if (ievent.code == ABS_Y) {
							speed = (ievent.value - ((Ymax - Ymin) / 2 )) * 255/(Ymax - Ymin);
						}
					}
					//print_event(&ievent);
					break;
			}
		}
		
		if (FD_ISSET(STDIN_FILENO, &writefds)) {
			//fprintf(stderr, "Write!\n");
		}
		
		
	}
/*
	Clean-up code
*/

	if (-1 == ioctl(inputfd,EVIOCGRAB,0)) {
		perror("ioctl EVIOCGRAB(0)");
	}
	fprintf(stderr,"released input device\n");
	close(inputfd);
	exit(1);
}
/*
#define BSIZE (10)

typedef struct {
	st_packet * buf;
	unsigned len;
	unsigned head;
	unsigned tail;
} pqueue;

pqueue queue;
unsigned sbsize = BSIZE;
if (!(queue.buf = malloc(sizeof(st_packet) * sbsize))) exit(1);
	for (int j = 0; j <= BSIZE; j++) {
		st_packet * p = &queue.buf[j];
		*p = st_command(j, j+1, j+2);
		printf("Address: %X, Command %X, Data %X, Checksum %X\n", p->addr, p->cmd, p->data, p->chksum);
	}
*/