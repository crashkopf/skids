
#include <stdint.h>

#include "intmath.h"
#include "sabertooth.h"
#include "timer.h"

#define DOATOMIC(stuff) do {timer_block(); stuff; timer_unblock();} while(0)

/*
	Control systems are complicated.  The commands to the Sabertooth may be 
	sent at a low baud rate, like 9600.  This is much slower than the 
	rate of commands coming in from the network or input device. We 
	don't want the motor to suddenly go from 10% duty cycle to 100% 
	duty cycle. In order to provide smoother control we introduce a 
	slew rate.  Commands from the input update the 	 setpoints for 
	speed and turn, but actual commanded values to the controller can 
	only change by x/T.
*/

// Currently commanded speed and turn
volatile int speedsp = 0; // positive is Forward
volatile int speedact = 0;
volatile int turnsp = 0;  // positive is to the right
volatile int turnact = 0;
volatile int ready = 0;
int slew = 100000; //143165576;

void j5setspeed(int sp) {
	DOATOMIC(speedsp = sp);
}
void j5setturn(int sp) {
	DOATOMIC(turnsp = sp);
}
int j5getspeed(void) {
	volatile int s;
	DOATOMIC(s = speedact);
	return s;
}
int j5getturn(void) {
	volatile int t;
	DOATOMIC(t = turnact);
	return t;
}
void j5update(void) {
	if (speedsp > speedact) speedact = iadd(speedact, slew, INT_MIN, speedsp);
	if (speedsp < speedact) speedact = iadd(speedact, -slew, speedsp, INT_MAX);

	if (turnsp > turnact) turnact = iadd(turnact, slew, turnact, turnsp);
	if (turnsp < turnact) turnact = iadd(turnact, -slew, turnsp, turnact);
	
	ready = 1;
}
int j5ready(void) {
	volatile int r;
	DOATOMIC(r = ready; ready = 0);
	return r;
}