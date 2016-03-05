/*
	jcmd.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/select.h>

#include "sabertooth.h"

#define xstr(s) str(s)
#define str(s) #s

int main (int argc, char * argv[]) {
	
	int8_t cmd = ST_FWD_B;
	int8_t val = 0;
	int8_t st = ST_START;
	
	st_packet p;
	
	if (argv[1]) val = atoi(argv[1]);
	
	write(STDOUT_FILENO, &st, 1);
	p = st_command(0, cmd, val);
	write(STDOUT_FILENO, &p, 4);
}