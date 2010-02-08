#ifndef HardwareSerial_h
#define HardwareSerial_h

#include <inttypes.h>
#include "Print.h"

class HardwareSerial : public Print
{
public:
	inline void begin(uint32_t baud, uint8_t txen_pin=255) {
		//_begin((F_CPU / 4 / baud - 1) / 2, pin);
		_begin(((F_CPU / 8) + (baud / 2)) / baud, txen_pin);
	}
	void _begin(uint16_t baud_count, uint8_t pin);
	void end(void);
	uint8_t available(void);
	int read(void);
	void flush(void);
	virtual void write(uint8_t);
	using Print::write;
};

#endif
