
TARGET=atmega328
FREQ=16000000
CFLAGS=-g -mmcu=$(TARGET) -DF_CPU=$(FREQ)UL
CC=avr-gcc
AVR_LD=avr-ld
OBJCOPY=avr-objcopy

ping.hex:

%.hex: %.elf
	$(OBJCOPY) -O ihex $< $@

%.c: %.h
	
ping.elf: ping.o serio.o buffer.o timer.o
	$(CC) $(CFLAGS) -o $@ $^

install: ping.hex
	avrdude -c arduino -p m328p -P /dev/ttyACM0 -U $<

clean:
	rm -f *.o ping.elf ping.hex
