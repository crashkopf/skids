/*
  buffer.c
*/

#include "buffer.h"

#define E_NULLPTR 0

int bflush(buffer * b) {
	b->len = 0;
	b->pos = 0;
	return 0;
}

// Points to one byte AFTER the end of data
int btail(buffer * b) {
	int p;
	p = b->pos + b->len;
	if (p >= b->size) 
		return (p - b->size);
	else
		return p;
}

int readb(buffer * b, char * d, unsigned int m) {
	unsigned int c;	// Current byte count
	if (!b || !d) 
		return E_NULLPTR;
	c = 0;
	while ((c < m) && (b->len)) {		// Loop until we get the requested data or run out
		d[c] = b->data[b->pos];		// Copy from head of buffer to destination
		b->pos++;			// Advance internal position
		if(b->pos >= b->size) 
			b->pos -= b->size;	// Wrap around if we hit the end of the array
		b->len--;			// Decrement buffer length
		c++;				// Increment count
	}
	return c;
}


int writeb(buffer * b, char * s, unsigned int m) {
	unsigned int c;  // Current byte count
	unsigned int p;  // Current offset
	if (!b || !s)
		return E_NULLPTR;
	if (b->len == b->size)
		return 0;	// Buffer is full
	c = 0;			// Clear the count
	p = btail(b);	// Start off at the tail of current data
	while ((c < m) && (b->len < b->size)) {
		b->data[p] = s[c];		// Copy from source to buffer
		p++;				// Advance to the next position
		if (p >= b->size) 
			p -= b->size;	// Wrap around if necessary
		b->len++;			// Increment length
		c++;				// Increment count
	}
	return c;
}
