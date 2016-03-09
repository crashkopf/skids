/*
	intmath.h - integer math convenience functions
*/

#ifndef INTMATH_H
#define INTMATH_H

#include <stdint.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif
// Subtract and limit the result
//~ inline
//~ int8_t int8_sublim (int8_t minuend, int8_t subtrahend, int8_t upper, int8_t lower) {
	//~ int8_t x;
	//~ if (minuend > upper) return upper;			// Minuend must already be < upper
	//~ if (minuend < lower) return lower;			// Minuend must already be > lower
	//~ if (subtrahend > minuend) return limit;		// Avoid unsigned underflow (result < 0)
	//~ x = minuend - subtrahend;
	//~ if (x > limit) return x;
	//~ else return limit;
//~ }
// Add and limit the result
static inline
int8_t int8_addlim (int8_t augend, int8_t addend, int8_t lower, int8_t upper) {
	if ((addend > 0) && ((upper - addend) < augend)) return upper;
	if ((addend < 0) && ((lower - addend) > augend)) return lower;
	return (addend + augend);
}
static inline
int iadd (int augend, int addend, int lower, int upper) {
	if ((addend > 0) && ((upper - addend) < augend)) return upper;
	if ((addend < 0) && ((lower - addend) > augend)) return lower;
	return (addend + augend);
}
// Subtract and limit the result
inline
unsigned u_sublim (unsigned minuend, unsigned subtrahend, unsigned limit) {
	unsigned x;
	if (minuend < limit) return limit;			// Minuend must already be > limit
	if (subtrahend > minuend) return limit;		// Avoid unsigned underflow (result < 0)
	x = minuend - subtrahend;
	if (x > limit) return x;
	else return limit;
}
// Add and limit the result
inline
unsigned u_addlim (unsigned augend, unsigned addend, unsigned limit) {
	unsigned x;
	if (augend > limit) return limit;			// Augend must be < limit
	if (addend > limit) return limit;			// Addend must be < limit
	x = augend + addend;
	if (x < addend) return limit;				// Overflow must have occurred (usually undefined behavior)
	if (x < limit) return x;
	else return limit;	
}
// Subtract and limit the result
inline
unsigned long ul_sublim (unsigned long minuend, unsigned long subtrahend, unsigned long limit) {
	unsigned long x;
	if (minuend < limit) return limit;			// Minuend must already be > limit
	if (subtrahend > minuend) return limit;		// Avoid unsigned underflow (result < 0)
	x = minuend - subtrahend;
	if (x > limit) return x;
	else return limit;
}
// Add and limit the result
inline
unsigned long ul_addlim (unsigned long augend, unsigned long addend, unsigned long limit) {
	unsigned long x;
	if (augend > limit) return limit;			// Augend must be < limit
	if (addend > limit) return limit;			// Addend must be < limit
	x = augend + addend;
	if (x < addend) return limit;				// Overflow must have occurred (usually undefined behavior)
	if (x < limit) return x;
	else return limit;	
}

#ifdef __cplusplus
}
#endif

#endif // INTMATH_H