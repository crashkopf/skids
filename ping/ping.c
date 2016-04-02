// ping.c - Sonar range finders

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>

#include <stdio.h>

#include "timer.h"
#include "serio.h"

#define T_PULSE (147)

timer_s forward;
timer_s down;
timer_s t;

unsigned char levels;

FILE sio;

ISR(PCINT0_vect) {
	unsigned char changed;
	changed = PINB ^ levels;  // See which bits got flipped since the last call to this interrupt.
	levels = PINB; // Save the current levels for the next time this interrupt fires
	// See if level changed for forward sensor
	if (changed & _BV(PB0)) {
		if (levels & _BV(PB0)) {
			// Input is high, so reset and start the timer
			timer_set(&forward, 0);
			timer_start(&forward);
		}
		else {
			// Input is low, so stop the timer
			timer_stop(&forward);
		}
	}
	
	// See if level changed for down sensor
	if (changed & _BV(PB1)) {
		if (levels & _BV(PB1)) {
			// Input is high, so reset and start the timer
			timer_set(&down, 0);
			timer_start(&down);
		}
		else {
			// Input is low, so stop the timer
			timer_stop(&down);
		}
	}
}

static int sio_putc(char c, FILE *stream) {SIO_write(&c, 1);}

static int sio_getc(FILE *stream) {char c; SIO_read(&c, 1); return c;}


void main (void) {
	// Enable sleep mode
	sleep_enable();
	// Enable interrupts
	sei();
	
	// Set up serial I/O
	SIO_init(19200);
	
	fdev_setup_stream(&sio, sio_putc, sio_getc, _FDEV_SETUP_RW);
	
	stdin = &sio;
	stdout = &sio;
	
	// Print the banner
	printf_P(PSTR("PEMS Autonomus Car Project - Sensor Control v0.1\n"));
	
	// Set up timers
	timer_init(50);
	timer_set(&t, 0);
	timer_start(&t);
	
	forward.direction = 1;
	down.direction = 1;

	PCICR |= _BV(PCIE0);
	PCMSK0 |= _BV(PCINT0);
	
	// Turn on blinky light pin
	DDRB |= _BV(DDB5);
	PORTB |= _BV(PB5);
	
	for(;;) {
		if (timer_read(&t) == 0) {
			timer_set(&t, 1000000);
			PORTB ^= _BV(PB5);
			printf_P(PSTR("F%03u D%03u\n"), timer_read(&forward) / 147, timer_read(&down));
		}
		sleep_cpu();
	}
}
