#ifndef SPI_DEVICE_H
#define SPI_DEVICE_H

#include <Arduino.h>
#include <SPI.h>

class SPI_Device {

public:

  // METHODS

  SPI_Device(SPISettings settings, pin_size_t cs_pin, pin_size_t irq_pin=0);
  void enter(void);
  void exit(void);
  uint8_t transfer(uint8_t b);

private:

  // MEMBERS

  SPISettings _settings;
  pin_size_t _cs_pin;
  pin_size_t _irq_pin;

}

#endif
