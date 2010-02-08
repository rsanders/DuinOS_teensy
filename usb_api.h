#ifndef USBserial_h_
#define USBserial_h_

#include <inttypes.h>

#include "Print.h"

class usb_serial_class : public Print
{
  public:
    void begin(long);
    void end(void);
    uint8_t available(void);
    int read(void);
    void flush(void);
    virtual void write(uint8_t);
    virtual void write(const uint8_t *buffer, uint16_t size);
    virtual void write(const char *str);
    void send_now(void);
    uint32_t baud(void);
    uint8_t stopbits(void);
    uint8_t paritytype(void);
    uint8_t numbits(void);
    uint8_t dtr(void);
    uint8_t rts(void);
};

extern usb_serial_class Serial;

#endif

