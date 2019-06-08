#ifndef SPI_DEVICE_H
#define SPI_DEVICE_H

#include <Arduino.h>
#include <SPI.h>

// FIXME: ArduinoCore-samd hasn't migrated to the new API yet
typedef uint8_t pin_size_t;

class SPI_Device {

public:

  // METHODS

  SPI_Device(SPISettings settings, pin_size_t cs_pin, pin_size_t irq_pin=0);
  void enter(void);
  void exit(void);
  uint8_t transfer(uint8_t b);
  void transfer(uint8_t *b, uint8_t count);

private:

  // MEMBERS

  SPISettings _settings;
  pin_size_t _cs_pin;
  pin_size_t _irq_pin;

};

#endif
