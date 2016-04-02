#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>
#include <limits.h>
#include "timer.h"

#define MS_COUNT (((F_CPU) / 1000000) - 1)

volatile timer_s * timer_head;

volatile unsigned resolution;

void timer_init(unsigned res) {
	// Use res to set our timer resolution in microseconds
	resolution = res;
	res = res * MS_COUNT;
	// Make sure timer head pointer is NULL
	timer_head = 0;
	// Initialize AVR timer
	// Zero count
	TCNT1 = 0;
	// Enable interrupts
	TIMSK1 = 0
		| _BV(OCIE1A)		// Output compare A interrupt enable
		| _BV(TOIE1)		// Overflow interrupt enable
	;
	// Timer control registers
	TCCR1A = 0;
	TCCR1B = 0
		| _BV(CS10)
		| _BV(WGM12)		// Clear TC on match
	;  // clk synchronous
	TCCR1C = 0;
	// Set output compare registers (write high byte first)
	OCR1AH = (unsigned char) (res >> 8);
	OCR1AL = (unsigned char) res;
}
void timer_start(timer_s *t) {
	volatile timer_s * u;
	// If there's no other timers, make this one the head
	if (!timer_head) timer_head = t;
	// Walk to the end of the list
	u = timer_head;
	while (u) {
		if (u == t) return; // Timer is already in the list
		if (u->next) u=u->next;
		else {
			u->next = t; // Found the end, add the new timer
			// Make sure t terminates the list
			t->next = 0;
			return;
		}
	}
}
void timer_stop(timer_s *t) {
	volatile timer_s * u = timer_head;
	// Check for t at the head
	if (timer_head == t) {
		// Unlink t from the list
		timer_head = t->next;
		t->next = 0;
		return;
	}
	// Walk the list looking for t
	while (u) {
		if (u->next == t) {
			// Unlink t from the list
			u->next = t->next;
			t->next = 0;
			return;
		}
		else u = u->next;
	}
}
void timer_set(timer_s *t, unsigned long c) {
	t->count = c;
}
unsigned long timer_read(timer_s *t) {
	return t->count;
}

ISR(TIMER1_COMPA_vect) {
	volatile timer_s * t = timer_head;
	unsigned tc1;
	while (t) {
		tc1 = TCNT1;
		if (t->direction){
			// Don't overflow
			if (t->count < ULONG_MAX - resolution)
				t->count += resolution;
			else
				t->count = ULONG_MAX;
		}
		else {
			// Don't underflow
			if (t->count > 0 + resolution)
				t->count -= resolution;
			else
				t->count = 0;
		}
		t=t->next;
	}
}

ISR(TIMER1_OVF_vect) {
}