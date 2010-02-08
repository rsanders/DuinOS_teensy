/* USB API for Teensy USB Development Board
 * http://www.pjrc.com/teensy/teensyduino.html
 * Copyright (c) 2008 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <avr/io.h>
#include <stdint.h>
#include "usb_private.h"
#include "usb_api.h"
#include "wiring.h"

// Public Methods //////////////////////////////////////////////////////////////

void usb_serial_class::begin(long speed)
{
	uint32_t t;

	// make sure USB is initialized
	usb_init();
	// wait for the host to finish enumeration
	// or for suspend mode to be detected
	while (1) {
		if (usb_configuration) {
			delay(200);  // a little time for host to load a driver
			return;
		}
		if (usb_suspended) {
			t = millis();
			while (1) {
				// must remain suspended for a while
				if (millis() - t > 500) {
					return;
				}
				if (!usb_suspended) break;
			}
		}
	}
}

void usb_serial_class::end(void)
{
	usb_shutdown();
	delay(25);
}

// number of bytes available in the receive buffer
uint8_t usb_serial_class::available(void)
{
        uint8_t n=0, i, intr_state;

        intr_state = SREG;
        cli();
        if (usb_configuration) {
                UENUM = CDC_RX_ENDPOINT;
                n = UEBCLX;
		if (!n) {
			i = UEINTX;
			if (i & (1<<RXOUTI) && !(i & (1<<RWAL))) UEINTX = 0x6B;
		}
        }
        SREG = intr_state;
        return n;
}

// get the next character, or -1 if nothing received
int usb_serial_class::read(void)
{
        uint8_t c, intr_state;

        // interrupts are disabled so these functions can be
        // used from the main program or interrupt context,
        // even both in the same program!
        intr_state = SREG;
        cli();
        if (!usb_configuration) {
                SREG = intr_state;
                return -1;
        }
        UENUM = CDC_RX_ENDPOINT;
	retry:
	c = UEINTX;
        if (!(c & (1<<RWAL))) {
                // no data in buffer
		if (c & (1<<RXOUTI)) {
			UEINTX = 0x6B;
			goto retry;
		}
                SREG = intr_state;
                return -1;
        }
        // take one byte out of the buffer
        c = UEDATX;
        // if this drained the buffer, release it
        if (!(UEINTX & (1<<RWAL))) UEINTX = 0x6B;
        SREG = intr_state;
        return c;
}

// discard any buffered input
void usb_serial_class::flush()
{
        uint8_t intr_state;

        if (usb_configuration) {
                intr_state = SREG;
                cli();
                UENUM = CDC_RX_ENDPOINT;
                while ((UEINTX & (1<<RWAL))) {
                        UEINTX = 0x6B;
                }
                SREG = intr_state;
        }
}
#if 0
// transmit a character.
void usb_serial_class::write(uint8_t c)
{
        uint8_t timeout, intr_state;

        // if we're not online (enumerated and configured), error
        if (!usb_configuration) return;
        // interrupts are disabled so these functions can be
        // used from the main program or interrupt context,
        // even both in the same program!
        intr_state = SREG;
        cli();
        UENUM = CDC_TX_ENDPOINT;
        // if we gave up due to timeout before, don't wait again
        if (transmit_previous_timeout) {
                if (!(UEINTX & (1<<RWAL))) {
                        SREG = intr_state;
                        return;
                }
                transmit_previous_timeout = 0;
        }
        // wait for the FIFO to be ready to accept data
        timeout = UDFNUML + TRANSMIT_TIMEOUT;
        while (1) {
                // are we ready to transmit?
                if (UEINTX & (1<<RWAL)) break;
                SREG = intr_state;
                // have we waited too long?  This happens if the user
                // is not running an application that is listening
                if (UDFNUML == timeout) {
                        transmit_previous_timeout = 1;
                        return;
                }
                // has the USB gone offline?
                if (!usb_configuration) return;
                // get ready to try checking again
                intr_state = SREG;
                cli();
                UENUM = CDC_TX_ENDPOINT;
        }
        // actually write the byte into the FIFO
        UEDATX = c;
        // if this completed a packet, transmit it now!
        if (!(UEINTX & (1<<RWAL))) UEINTX = 0x3A;
        transmit_flush_timer = TRANSMIT_FLUSH_TIMEOUT;
        SREG = intr_state;
}
#endif

void usb_serial_class::write(uint8_t c)
{
	write(&c, 1);
}

// transmit a block of data
void usb_serial_class::write(const uint8_t *buffer, uint16_t size)
{
	uint8_t timeout, intr_state, write_size;

	// if we're not online (enumerated and configured), error
	if (!usb_configuration) return;
	// interrupts are disabled so these functions can be
	// used from the main program or interrupt context,
	// even both in the same program!
	intr_state = SREG;
	cli();
	UENUM = CDC_TX_ENDPOINT;
	// if we gave up due to timeout before, don't wait again
	if (transmit_previous_timeout) {
		if (!(UEINTX & (1<<RWAL))) {
			SREG = intr_state;
			return;
		}
		transmit_previous_timeout = 0;
	}
	// each iteration of this loop transmits a packet
	while (size) {
		// wait for the FIFO to be ready to accept data
		timeout = UDFNUML + TRANSMIT_TIMEOUT;
		while (1) {
			// are we ready to transmit?
			if (UEINTX & (1<<RWAL)) break;
			SREG = intr_state;
			// have we waited too long?  This happens if the user
			// is not running an application that is listening
			if (UDFNUML == timeout) {
				transmit_previous_timeout = 1;
				return;
			}
			// has the USB gone offline?
			if (!usb_configuration) return;
			// get ready to try checking again
			intr_state = SREG;
			cli();
			UENUM = CDC_TX_ENDPOINT;
		}

		// compute how many bytes will fit into the next packet
		write_size = CDC_TX_SIZE - UEBCLX;
		if (write_size > size) write_size = size;
		size -= write_size;

#define ASM_COPY1(src, dest, tmp) "ld " tmp ", " src "\n\t" "st " dest ", " tmp "\n\t"
#define ASM_COPY2(src, dest, tmp) ASM_COPY1(src, dest, tmp) ASM_COPY1(src, dest, tmp)
#define ASM_COPY4(src, dest, tmp) ASM_COPY2(src, dest, tmp) ASM_COPY2(src, dest, tmp)
#define ASM_COPY8(src, dest, tmp) ASM_COPY4(src, dest, tmp) ASM_COPY4(src, dest, tmp)

#if 1
		// write the packet
		do {
			uint8_t tmp;
			asm volatile(
			"L%=begin:"					"\n\t"
				"ldi	r30, %4"			"\n\t"
				"sub	r30, %3"			"\n\t"
				"cpi	r30, %4"			"\n\t"
				"brsh	L%=err"				"\n\t"
				"lsl	r30"				"\n\t"
				"clr	r31"				"\n\t"
				"subi	r30, lo8(-(pm(L%=table)))"	"\n\t"
				"sbci	r31, hi8(-(pm(L%=table)))"	"\n\t"
				"ijmp"					"\n\t"
			"L%=err:"					"\n\t"
				"rjmp	L%=end"				"\n\t"
			"L%=table:"					"\n\t"
				#if (CDC_TX_SIZE == 64)
				ASM_COPY8("Y+", "X", "%1")
				ASM_COPY8("Y+", "X", "%1")
				ASM_COPY8("Y+", "X", "%1")
				ASM_COPY8("Y+", "X", "%1")
				#endif
				#if (CDC_TX_SIZE >= 32)
				ASM_COPY8("Y+", "X", "%1")
				ASM_COPY8("Y+", "X", "%1")
				#endif
				#if (CDC_TX_SIZE >= 16)
				ASM_COPY8("Y+", "X", "%1")
				#endif
				ASM_COPY8("Y+", "X", "%1")
			"L%=end:"					"\n\t"
				: "+y" (buffer), "=r" (tmp)
				: "0" (buffer), "r" (write_size), "M" (CDC_TX_SIZE), "x" (&UEDATX)
				: "r30", "r31"
			);
		} while (0);
#endif
#if 0
		switch (write_size) {
			#if (CDC_TX_SIZE == 64)
			case 64: UEDATX = *buffer++;
			case 63: UEDATX = *buffer++;
			case 62: UEDATX = *buffer++;
			case 61: UEDATX = *buffer++;
			case 60: UEDATX = *buffer++;
			case 59: UEDATX = *buffer++;
			case 58: UEDATX = *buffer++;
			case 57: UEDATX = *buffer++;
			case 56: UEDATX = *buffer++;
			case 55: UEDATX = *buffer++;
			case 54: UEDATX = *buffer++;
			case 53: UEDATX = *buffer++;
			case 52: UEDATX = *buffer++;
			case 51: UEDATX = *buffer++;
			case 50: UEDATX = *buffer++;
			case 49: UEDATX = *buffer++;
			case 48: UEDATX = *buffer++;
			case 47: UEDATX = *buffer++;
			case 46: UEDATX = *buffer++;
			case 45: UEDATX = *buffer++;
			case 44: UEDATX = *buffer++;
			case 43: UEDATX = *buffer++;
			case 42: UEDATX = *buffer++;
			case 41: UEDATX = *buffer++;
			case 40: UEDATX = *buffer++;
			case 39: UEDATX = *buffer++;
			case 38: UEDATX = *buffer++;
			case 37: UEDATX = *buffer++;
			case 36: UEDATX = *buffer++;
			case 35: UEDATX = *buffer++;
			case 34: UEDATX = *buffer++;
			case 33: UEDATX = *buffer++;
			#endif
			#if (CDC_TX_SIZE >= 32)
			case 32: UEDATX = *buffer++;
			case 31: UEDATX = *buffer++;
			case 30: UEDATX = *buffer++;
			case 29: UEDATX = *buffer++;
			case 28: UEDATX = *buffer++;
			case 27: UEDATX = *buffer++;
			case 26: UEDATX = *buffer++;
			case 25: UEDATX = *buffer++;
			case 24: UEDATX = *buffer++;
			case 23: UEDATX = *buffer++;
			case 22: UEDATX = *buffer++;
			case 21: UEDATX = *buffer++;
			case 20: UEDATX = *buffer++;
			case 19: UEDATX = *buffer++;
			case 18: UEDATX = *buffer++;
			case 17: UEDATX = *buffer++;
			#endif
			#if (CDC_TX_SIZE >= 16)
			case 16: UEDATX = *buffer++;
			case 15: UEDATX = *buffer++;
			case 14: UEDATX = *buffer++;
			case 13: UEDATX = *buffer++;
			case 12: UEDATX = *buffer++;
			case 11: UEDATX = *buffer++;
			case 10: UEDATX = *buffer++;
			case  9: UEDATX = *buffer++;
			#endif
			case  8: UEDATX = *buffer++;
			case  7: UEDATX = *buffer++;
			case  6: UEDATX = *buffer++;
			case  5: UEDATX = *buffer++;
			case  4: UEDATX = *buffer++;
			case  3: UEDATX = *buffer++;
			case  2: UEDATX = *buffer++;
			default:
			case  1: UEDATX = *buffer++;
			case  0: break;
		}
#endif
		// if this completed a packet, transmit it now!
		if (!(UEINTX & (1<<RWAL))) UEINTX = 0x3A;
		transmit_flush_timer = TRANSMIT_FLUSH_TIMEOUT;
	}
	SREG = intr_state;
}

// transmit a string
void usb_serial_class::write(const char *str)
{
	uint16_t size=0;
	const char *p=str;

	while (*p++) size++;
	if (size) write((const uint8_t *)str, size);
}

// These are Teensy-specific extensions to the Serial object

// immediately transmit any buffered output.
// This doesn't actually transmit the data - that is impossible!
// USB devices only transmit when the host allows, so the best
// we can do is release the FIFO buffer for when the host wants it
void usb_serial_class::send_now(void)
{
        uint8_t intr_state;

        intr_state = SREG;
        cli();
        if (usb_configuration && transmit_flush_timer) {
                UENUM = CDC_TX_ENDPOINT;
                UEINTX = 0x3A;
                transmit_flush_timer = 0;
        }
        SREG = intr_state;
}

uint32_t usb_serial_class::baud(void)
{
	return *(uint32_t *)cdc_line_coding;
}

uint8_t usb_serial_class::stopbits(void)
{
	return cdc_line_coding[4];
}

uint8_t usb_serial_class::paritytype(void)
{
	return cdc_line_coding[5];
}

uint8_t usb_serial_class::numbits(void)
{
	return cdc_line_coding[6];
}

uint8_t usb_serial_class::dtr(void)
{
	return (cdc_line_rtsdtr & USB_SERIAL_DTR) ? 1 : 0;
}

uint8_t usb_serial_class::rts(void)
{
	return (cdc_line_rtsdtr & USB_SERIAL_RTS) ? 1 : 0;
}



// Preinstantiate Objects //////////////////////////////////////////////////////

usb_serial_class Serial = usb_serial_class();

