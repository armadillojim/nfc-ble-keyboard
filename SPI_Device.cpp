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
SPI_Device::enter() {
  SPI.beginTransaction(_settings);
  digitalWrite(_cs_pin, LOW);
}
SPI_Device::exit() {
  digitalWrite(_cs_pin, HIGH);
  SPI.endTransaction();
}
SPI_Device::transfer(uint8_t b) {
  return SPI.transfer(b);
}
