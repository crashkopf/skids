/*
	jcmd.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/select.h>

#include "sabertooth.h"

int main (int argc, char * argv[]) {
	int8_t cmd, val = 0;
	int off = 0;
	
	unsigned char buf[32];
	
	if (argv[1]) cmd = strtol(argv[1], NULL, 0);
	if (argv[2]) val = strtol(argv[2], NULL, 0);
	
	buf[off++] = ST_START;
	off += st_command(&buf[1], 0, cmd, val);
	
	write(STDOUT_FILENO, &buf, off);
}