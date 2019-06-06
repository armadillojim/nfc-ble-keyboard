#include <stdexcept>

#include "PN532.h"

const SPISettings pn532_spi_settings(1000000, LSBFIRST, SPI_MODE0);

PN532::PN532(pin_size_t cs_pin, bool debug) : _debug(debug) {
  _spi = SPI_Device(pn532_spi_settings, cs_pin);
  // try to talk to the PN532
  _wakeup();
  uint32_t version;
  bool success = get_firmware_version(&version);
  if (success) { return; }
  // first time often fails; if so, try again
  success = get_firmware_version(&version);
  if (success) { return; }
  else {
    throw std::runtime_error(F("Could not wake PN532"));
    return;
  }
}

void PN532::_wakeup(void) {
  // set the CS pin to wake the PN532, and wait for it to wake
  _spi.enter();
  delay(WAKE_DELAY);
  _spi.exit();
}

bool PN532::_wait_ready(uint16_t timeout) {
  uint16_t timer = 0;
  bool ready = false;
  uint8_t status;
  _spi.enter();
  while (!ready && timer < timeout) {
    status = _spi.transfer(SPI_STATUSREAD);
    ready = status & SPI_READY;
    if (!ready) {
      delay(SPI_STATUSPOLLINTERVAL);
      timer += SPI_STATUSPOLLINTERVAL;
    }
  }
  _spi.exit();
  return ready;
}

void PN532::_read_data(uint8_t * data, uint8_t count) {
  _spi.enter();
  _spi.transfer(SPI_DATAREAD);
  _spi.transfer(data, count);
  _spi.exit();
}

void PN532::_write_data(uint8_t * data, uint8_t count) {
  _spi.enter();
  _spi.transfer(SPI_DATAWRITE);
  _spi.transfer(data, count);
  _spi.exit();
}

uint8_t PN532::_read_frame(uint8_t * response, uint8_t * count) {
  const uint8_t frame_length = count+8;
  uint8_t i, checksum, frame[frame_length];
  _read_data(frame, frame_length);
  if (_debug) {
    Serial.print(F("Read frame: "));
    for (i=0; i<frame_length, i++) { Serial.print(frame[i], HEX); }
  }
  // Swallow all the 0x00 values that precede 0xFF
  // NB: sometimes frames are missing the preamble
  uint8_t offset = 0;
  while (frame[offset] == 0x00) {
    offset += 1;
    if (offset == frame_length) { return ERR_BAD_PREAMBLE; }
  }
  if (frame[offset] != 0xFF) { return ERR_BAD_PACKET_START; }
  offset += 1;
  if (offset == frame_length) { return ERR_EMPTY_PACKET; }
  // Validate length
  uint8_t response_length = frame[offset];
  if (response_length == 0) { return ERR_EMPTY_RESPONSE; }
  if (response_length + frame[offset+1] != 0) { return ERR_BAD_LENGTH_CHECKSUM; }
  offset += 2;
  // Make sure the claimed response length doesn't overrun the read length (with
  // two bytes for data checksum and postamble)
  if (response_length + offset > frame_length - 2) { return ERR_LONG_RESPONSE; }
  // Validate response data checksum (NB: +1 to include DCS)
  for (i=offset, checksum=0; i<offset+response_length+1; i++) {
    checksum += frame[i];
  }
  if (checksum != 0) { return ERR_BAD_DATA_CHECKSUM; }
  // Check first and last bytes: did this frame come from the PN532? is there a postamble?
  if (frame[offset] != PN532TOHOST) { return ERR_BAD_TFI; }
  if (frame[offset+response_length+1] != POSTAMBLE) { return ERR_BAD_POSTAMBLE; }
  // Return frame data
  if (response_length > count) {
    // NB: this is necessary because the preamble and part of the start code may be missing
    return ERR_LONG_RESPONSE;
  }
  count = response_length;
  for (i=0; i<response_length; i++) { response[i] = frame[offset+1+i]; }
  return SUCCESS;
}

void PN532::_write_frame(uint8_t * data, uint8_t count) {
  const uint8_t frame_length = count+8;
  uint8_t i, checksum, frame[frame_length];
  frame[0] = PREAMBLE;
  frame[1] = STARTCODE1;
  frame[2] = STARTCODE2;
  frame[3] = count + 1;
  frame[4] = ~count;
  frame[5] = HOSTTOPN532;
  for (i=0, checksum = HOSTTOPN532; i<count; i++) {
    frame[6+i] = data[i];
    checksum += data[i];
  }
  frame[6+count] = ~checksum + 1;
  frame[7+count] = POSTAMBLE;
  if (_debug) {
    Serial.print(F("Writing frame: "));
    for (i=0; i<frame_length, i++) { Serial.print(frame[i], HEX); }
    Serial.println();
  }
  _write_data(frame, frame_length);
}

uint8_t PN532::_call_function(uint8_t command, uint8_t * params, uint8_t params_len, uint8_t * response, uint8_t * response_len, uint16_t timeout) {
  uint8_t i, status, command_data[1+params_len], ack_p[ACK_LEN], response_data[1+(*response_len)];
  try {
    // write the command data to the PN532
    command_data[0] = command;
    for (i=0; i<params_len; i++) { command_data[i+1] = params[i]; }
    _write_frame(command_data, 1+params_len);
    // wait for ACK
    if (!_wait_ready(timeout)) {
      status = ERR_ACK_TIMEOUT;
      throw std::runtime_error(F("timed out waiting for command ACK"));
    }
    _read_data(ack_p, ACK_LEN);
    status = SUCCESS;
    for (i=0; i<ACK_LEN; i++) {
      if (ack_p[i] != ACK_FRAME[i]) { status = ERR_NO_COMMAND_ACK; }
    }
    if (status) { throw std::runtime_error(F("no ACK after command")); }
    // wait for response
    if (!_wait_ready(timeout)) {
      status = ERR_RESPONSE_TIMEOUT;
      throw std::runtime_error(F("timed out waiting for command response"));
    }
    *response_len += 1
    status = _read_frame(response_data, response_len);
    if (status) { throw std::runtime_error(F("bad command response frame")); }
    if (response_data[0] != command+1) {
      status = ERR_WRONG_COMMAND_RESPONSE;
      throw std::runtime_error(F("wrong command response"));
    }
    // return the response
    *response_len -= 1;
    for (i=0; i<response_len; i++) { response[i] = response_data[i+1]; }
    return SUCCESS;
  } catch( const std::runtime_error& e ) {
    // something went wrong while trying the command:
    // * send an ACK to abort (and reset communications)
    // * return the bad status code
    for (i=0; i<ACK_LEN; i++) { ack_p[i] = ACK_FRAME[i]; }
    _write_data(ack_p, ACK_LEN);
    return status;
  }
}

bool PN532::get_firmware_version(uint32_t * version) {
  uint8_t version_length = 4;
  uint8_t status = _call_function(COMMAND_GETFIRMWAREVERSION, nullptr, 0, version, &version_length, 500);
  if (_debug && status) {
    Serial.print(F("COMMAND_GETFIRMWAREVERSION failed: "));
    Serial.print(status, HEX);
    Serial.print(F(" "));
    Serial.println(*version, HEX);
  }
  return (status == SUCCESS && version_length == 4);
}
