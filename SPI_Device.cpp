#include "SPI_Device.h"

SPI_Device::SPI_Device(SPISettings settings, pin_size_t cs_pin, pin_size_t irq_pin) : _settings(settings), _cs_pin(cs_pin), _irq_pin(irq_pin) {
  // set up interrupts
  if (_irq_pin) {
    pinMode(_irq_pin, INPUT);
    // UNIMPLEMENTED
    // set an interrupt handler:
    // attachInterrupt(digitalPinToInterrupt(_irq_pin), my_interrupt_handler, FALLING);
    // my_interrupt_handler should catch ACKs and responses after a command has been issued
    // if an expected interrupt isn't received before timeout, send an abort (ACK)
  }
  // Set CS pin to output and de-assert by default
  pinMode(_cs_pin, OUTPUT);
  digitalWrite(_cs_pin, HIGH);
}
void SPI_Device::enter(void) {
  SPI.beginTransaction(_settings);
  digitalWrite(_cs_pin, LOW);
}
void SPI_Device::exit(void) {
  digitalWrite(_cs_pin, HIGH);
  SPI.endTransaction();
  // FIXME: this shouldn't be necessary, but it is.  Some possible causes:
  // * the SPI bus and/or the PN532 doesn't like the CS pin being toggled
  //   quickly between the _wait_ready() and _read_data() calls.  Fix: integrate
  //   _read_data() and _write_data() into their callers (and push _enter() and
  //   _exit() to the top level).  That is, we don't have to support UART/HSU and
  //   I2C interfaces so no need to abstract that out.
  // * the PN532 prefers using the IRQ pin to signal readiness, and the SPI
  //   interface isn't perfectly implemented.  Fix: switch to using the IRQ pin.
  delay(1);
}
uint8_t SPI_Device::transfer(uint8_t b) {
  return SPI.transfer(b);
}
void SPI_Device::transfer(uint8_t *b, uint8_t count) {
  SPI.transfer(b, count);
}
