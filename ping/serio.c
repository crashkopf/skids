/*
  serio.c
*/
#include <avr/io.h>
#include <avr/interrupt.h>

#include "serio.h"
#include "buffer.h"

#ifndef BAUD_TOL
  #define BAUD_TOL 2
#endif
#define BSIZE 80


char rdata[BSIZE];
char wdata[BSIZE];
buffer rdbuf = {0, 0, BSIZE, rdata};
buffer wrbuf = {0, 0, BSIZE, wdata};

static inline void txstart(void) {
	UCSR0B |= _BV(UDRIE0);		// Turn on UDR empty interrupt
}
static inline void txstop(void) {
	UCSR0B &= ~(_BV(UDRIE0));	// Turn off UDR empty interrupt
}
static inline void rxstart(void) {
	UCSR0B |= _BV(RXCIE0);		// Turn on RX complete interrupt
}
static inline void rxstop(void) {
	UCSR0B &= ~(_BV(RXCIE0));	// Turn off RX complete interrupt
}

ISR(USART_RX_vect) {
	char c;
	//int n;
	c = UDR0; // Always read from UDR, otherwise the interrupt will keep firing
	//n = writeb(&rdbuf, &c, 1);
	writeb(&rdbuf, &c, 1);
}

ISR(USART_UDRE_vect) {
	char c;
	int n;
	n = readb(&wrbuf, &c, 1);
	if (n > 0) UDR0 = c;
	else txstop();  // Turn off interrupt when the buffer is empty, otherwise it will keep firing
}

int SIO_read(char * s, unsigned int m) {
	int n;
	rxstop();			// Stop RX so buffer isn't modified
	n = readb(&rdbuf, s, m);	// Read from buffer
	rxstart();			// Start RX again
	return n;
}

int SIO_write(char * s, unsigned int m) {
	int n;
	txstop();			// Stop TX so buffer isn't modified
	n = writeb(&wrbuf, s, m);	// Write to buffer
	txstart();			// Start TX again
	return n;
}

/*
 * Initialize the UART
 * Sets up as baud N81
 */
void SIO_init(unsigned long baud) {

	unsigned div;
	unsigned long actual;
	int error;

	// First, we calculate the divisor for baud clock generation, e.g. 9600 baud is a divisor of 103
	div = ((F_CPU) / (16 * baud)) - 1;
	
	// We have to calculate the actual baud generated by the divisor, e.g. a divisor of 103 is 9615 baud
	actual = (F_CPU) / (16 * (div + 1));
	
	// Calculate the percent error
	error = ((actual * 100 / div) - 100);
	
	// Check to see if this will put us within the tolerance band for the desire baud rate
	if (error > (BAUD_TOL) || error < -(BAUD_TOL)) {
		// Recalculate div to get a better value at 2X clock rate.
		div = ((F_CPU) / (8 * baud)) - 1;
		// Turn on 2X clock
		UCSR0A = _BV(U2X0);
	}
	UBRR0L = (unsigned char) div;
	UBRR0H = (unsigned char) div >> 8;

	UCSR0B |= _BV(TXEN0) | _BV(RXEN0); /* tx/rx enable */

	UCSR0C = _BV(UCSZ00) | _BV(UCSZ01); // 8-bit data (default)
	
	rxstart(); // enable interrupts
	txstart();
}
