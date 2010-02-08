#ifndef usb_serial_h__
#define usb_serial_h__

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

/**************************************************************************
 *
 *  Configurable Options
 *
 **************************************************************************/

// You can change these to give your code its own name.  On Windows,
// these are only used before an INF file (driver install) is loaded.
#define STR_MANUFACTURER        L"Teensyduino"
#define STR_PRODUCT             L"USB Serial"

// All USB serial devices are supposed to have a serial number
// (according to Microsoft).  On windows, a new COM port is created
// for every unique serial/vendor/product number combination.  If
// you program 2 identical boards with 2 different serial numbers
// and they are assigned COM7 and COM8, each will always get the
// same COM port number because Windows remembers serial numbers.
//
// On Mac OS-X, a device file is created automatically which
// incorperates the serial number, eg, /dev/cu-usbmodem12341
//
// Linux by default ignores the serial number, and creates device
// files named /dev/ttyACM0, /dev/ttyACM1... in the order connected.
// Udev rules (in /etc/udev/rules.d) can define persistent device
// names linked to this serial number, as well as permissions, owner
// and group settings.
#define STR_SERIAL_NUMBER       L"12345"

// Mac OS-X and Linux automatically load the correct drivers.  On
// Windows, even though the driver is supplied by Microsoft, an
// INF file is needed to load the driver.  These numbers need to
// match the INF file.
#define VENDOR_ID               0x16C0
#define PRODUCT_ID              0x0483

// When you write data, it goes into a USB endpoint buffer, which
// is transmitted to the PC when it becomes full, or after a timeout
// with no more writes.  Even if you write in exactly packet-size
// increments, this timeout is used to send a "zero length packet"
// that tells the PC no more data is expected and it should pass
// any buffered data to the application that may be waiting.  If
// you want data sent immediately, call usb_serial_flush_output().
#define TRANSMIT_FLUSH_TIMEOUT  3   /* in milliseconds */

// If the PC is connected but not "listening", this is the length
// of time before usb_serial_getchar() returns with an error.  This
// is roughly equivilant to a real UART simply transmitting the
// bits on a wire where nobody is listening, except you get an error
// code which you can ignore for serial-like discard of data, or
// use to know your data wasn't sent.
#define TRANSMIT_TIMEOUT        15   /* in milliseconds */


/**************************************************************************
 *
 *  Endpoint Buffer Configuration
 *
 **************************************************************************/

// These buffer sizes are best for most applications, but perhaps if you
// want more buffering on some endpoint at the expense of others, this
// is where you can make such changes.  The AT90USB162 has only 176 bytes
// of DPRAM (USB buffers) and only endpoints 3 & 4 can double buffer.

#define ENDPOINT0_SIZE          32
#define CDC_ACM_ENDPOINT        2
#define CDC_ACM_SIZE            8
#define CDC_ACM_BUFFER          EP_SINGLE_BUFFER
#define CDC_RX_ENDPOINT         3
#define CDC_RX_BUFFER           EP_DOUBLE_BUFFER
#define CDC_TX_ENDPOINT         4
#define CDC_TX_BUFFER           EP_DOUBLE_BUFFER

#if defined(__AVR_AT90USB162__)
#define CDC_RX_SIZE             32
#define CDC_TX_SIZE             32
#elif defined(__AVR_ATmega32U4__) || defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
#define CDC_RX_SIZE             64
#define CDC_TX_SIZE             64
#endif




// setup
void usb_init(void);			// initialize everything
void usb_shutdown(void);		// shut off USB

// zero when we are not configured, non-zero when enumerated
extern volatile uint8_t usb_configuration;
extern volatile uint8_t usb_suspended;

// the time remaining before we transmit any partially full
// packet, or send a zero length packet.
extern volatile uint8_t transmit_flush_timer;
extern uint8_t transmit_previous_timeout;

// serial port settings (baud rate, control signals, etc) set
// by the PC.  These are ignored, but kept in RAM because the
// CDC spec requires a read that returns the current settings.
extern volatile uint8_t cdc_line_coding[7];
extern volatile uint8_t cdc_line_rtsdtr;



// constants corresponding to the various serial parameters
#define USB_SERIAL_DTR                  0x01
#define USB_SERIAL_RTS                  0x02
#define USB_SERIAL_1_STOP               0
#define USB_SERIAL_1_5_STOP             1
#define USB_SERIAL_2_STOP               2
#define USB_SERIAL_PARITY_NONE          0
#define USB_SERIAL_PARITY_ODD           1
#define USB_SERIAL_PARITY_EVEN          2
#define USB_SERIAL_PARITY_MARK          3
#define USB_SERIAL_PARITY_SPACE         4
#define USB_SERIAL_DCD                  0x01
#define USB_SERIAL_DSR                  0x02
#define USB_SERIAL_BREAK                0x04
#define USB_SERIAL_RI                   0x08
#define USB_SERIAL_FRAME_ERR            0x10
#define USB_SERIAL_PARITY_ERR           0x20
#define USB_SERIAL_OVERRUN_ERR          0x40


// Everything below this point is only intended for usb_serial.c
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#define EP_TYPE_CONTROL			0x00
#define EP_TYPE_BULK_IN			0x81
#define EP_TYPE_BULK_OUT		0x80
#define EP_TYPE_INTERRUPT_IN		0xC1
#define EP_TYPE_INTERRUPT_OUT		0xC0
#define EP_TYPE_ISOCHRONOUS_IN		0x41
#define EP_TYPE_ISOCHRONOUS_OUT		0x40
#define EP_SINGLE_BUFFER		0x02
#define EP_DOUBLE_BUFFER		0x06
#define EP_SIZE(s)	((s) == 64 ? 0x30 :	\
			((s) == 32 ? 0x20 :	\
			((s) == 16 ? 0x10 :	\
			             0x00)))

#define MAX_ENDPOINT		4

#define LSB(n) (n & 255)
#define MSB(n) ((n >> 8) & 255)

#if defined(__AVR_AT90USB162__)
#define HW_CONFIG() 
#define PLL_CONFIG() (PLLCSR = ((1<<PLLE)|(1<<PLLP0)))
#define USB_CONFIG() (USBCON = (1<<USBE))
#define USB_FREEZE() (USBCON = ((1<<USBE)|(1<<FRZCLK)))
#elif defined(__AVR_ATmega32U4__)
#define HW_CONFIG() (UHWCON = 0x01)
#define PLL_CONFIG() (PLLCSR = 0x12)
#define USB_CONFIG() (USBCON = ((1<<USBE)|(1<<OTGPADE)))
#define USB_FREEZE() (USBCON = ((1<<USBE)|(1<<FRZCLK)))
#elif defined(__AVR_AT90USB646__)
#define HW_CONFIG() (UHWCON = 0x81)
#define PLL_CONFIG() (PLLCSR = 0x1A)
#define USB_CONFIG() (USBCON = ((1<<USBE)|(1<<OTGPADE)))
#define USB_FREEZE() (USBCON = ((1<<USBE)|(1<<FRZCLK)))
#elif defined(__AVR_AT90USB1286__)
#define HW_CONFIG() (UHWCON = 0x81)
#define PLL_CONFIG() (PLLCSR = 0x16)
#define USB_CONFIG() (USBCON = ((1<<USBE)|(1<<OTGPADE)))
#define USB_FREEZE() (USBCON = ((1<<USBE)|(1<<FRZCLK)))
#endif

// standard control endpoint request types
#define GET_STATUS			0
#define CLEAR_FEATURE			1
#define SET_FEATURE			3
#define SET_ADDRESS			5
#define GET_DESCRIPTOR			6
#define GET_CONFIGURATION		8
#define SET_CONFIGURATION		9
#define GET_INTERFACE			10
#define SET_INTERFACE			11
// CDC (communication class device)
#define CDC_SET_LINE_CODING		0x20
#define CDC_GET_LINE_CODING		0x21
#define CDC_SET_CONTROL_LINE_STATE	0x22
#define CDC_SEND_BREAK			0x23

#define pgm_read_byte_postinc(val, addr) \
	asm ("lpm  %0, Z+\n" : "=r" (val), "+z" (addr) : )
#define pgm_read_word_postinc(val, addr) \
	asm ("lpm  %A0, Z+\n\tlpm  %B0, Z+\n" : "=r" (val), "+z" (addr) : )
#define read_word_lsbfirst(val, reg) \
	asm volatile("lds  %A0, %1\n\tlds  %B0, %1\n" : "=r" (val) : "M" ((int)(&reg)) )





#ifdef __cplusplus
} // extern "C"
#endif

#endif
