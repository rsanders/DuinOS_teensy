/* -*- mode: jde; c-basic-offset: 2; indent-tabs-mode: nil -*- */

/*
  Part of the Wiring project - http://wiring.uniandes.edu.co

  Copyright (c) 2004-05 Hernando Barragan

  Modified for Teensyduino by Paul Stoffregen, paul@pjrc.com
  http://www.pjrc.com/teensy/teensyduino.html

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA
  
  Modified 24 November 2006 by David A. Mellis
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>

#include "wiring.h"
#include "wiring_private.h"

#if defined(__AVR_ATmega32U4__)
#define NUM_INTERRUPT 4
#else
#define NUM_INTERRUPT 8
#endif

volatile static voidFuncPtr intFunc[NUM_INTERRUPT];

static const uint8_t PROGMEM interrupt_mode_mask[] = {0xFC, 0xF3, 0xCF, 0x3F};

#if defined(__AVR_ATmega32U4__)
void attachInterrupt(uint8_t interruptNum, void (*userFunc)(void), uint8_t mode)
{
	uint8_t mask;

	if (interruptNum >= NUM_INTERRUPT) return;
	intFunc[interruptNum] = userFunc;
	mask = pgm_read_byte(interrupt_mode_mask + interruptNum);
	mode &= 0x03;
	EICRA = (EICRA & mask) | (mode << (interruptNum * 2));
	EIMSK |= (1 << interruptNum);
}
#else
void attachInterrupt(uint8_t interruptNum, void (*userFunc)(void), uint8_t mode)
{
	uint8_t mask, index;

	if (interruptNum >= NUM_INTERRUPT) return;
	intFunc[interruptNum] = userFunc;
	index = interruptNum & 3;
	mask = pgm_read_byte(interrupt_mode_mask + index);
	mode &= 0x03;
	if (interruptNum & 4) {
		EICRB = (EICRB & mask) | (mode << (index * 2));
	} else {
		EICRA = (EICRA & mask) | (mode << (index * 2));
	}
	EIMSK |= (1 << interruptNum);
}
#endif

void detachInterrupt(uint8_t interruptNum)
{
	if (interruptNum >= NUM_INTERRUPT) return;

	EIMSK &= ~(1 << interruptNum);
	intFunc[interruptNum] = 0;
}

SIGNAL(INT0_vect) {
	if (intFunc[0]) intFunc[0]();	// INT0 is pin 0 (PD0)
}
SIGNAL(INT1_vect) {
	if (intFunc[1]) intFunc[1]();	// INT1 is pin 1 (PD1)
}
SIGNAL(INT2_vect) {
	if (intFunc[2]) intFunc[2]();	// INT2 is pin 2 (PD2) (also Serial RX)
}
SIGNAL(INT3_vect) {
	if (intFunc[3]) intFunc[3]();	// INT3 is pin 3 (PD3) (also Serial TX)
}
#if !defined(__AVR_ATmega32U4__)
SIGNAL(INT4_vect) {
	if (intFunc[4]) intFunc[4]();	// INT4 is pin 20 (PC7)
}
SIGNAL(INT5_vect) {
	if (intFunc[5]) intFunc[5]();	// INT5 is pin 4 (PD4)
}
SIGNAL(INT6_vect) {
	if (intFunc[6]) intFunc[6]();	// INT6 is pin 6 (PD6)
}
SIGNAL(INT7_vect) {
	if (intFunc[7]) intFunc[7]();	// INT7 is pin 7 (PD7)
}
#endif

