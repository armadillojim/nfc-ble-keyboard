#include "PN532.h"

const uint8_t PN532::ACK_FRAME[]   = { 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00 };
const uint8_t PN532::NACK_FRAME[]  = { 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00 };
const uint8_t PN532::ERROR_FRAME[] = { 0x00, 0x00, 0xFF, 0x01, 0xFF, 0x7F, 0x81, 0x00 };

const SPISettings PN532::_spi_settings(1000000, LSBFIRST, SPI_MODE0);

// Serial.print HEX doesn't offer an option to pad left with 0s
void _print_uint8_hex(uint8_t b) {
  if (b < 0x10) {
    Serial.print("0");
  }
  Serial.print(b, HEX);
}

PN532::PN532(pin_size_t cs_pin, bool debug) : _debug(debug), _spi(_spi_settings, cs_pin) {
  // try to talk to the PN532
  _wakeup();
  uint32_t version;
  bool success = get_firmware_version(&version);
  if (success) { _wake_success = true; return; }
  // first time often fails; if so, try again
  success = get_firmware_version(&version);
  if (success) { _wake_success = true; return; }
  else {
    // throw std::runtime_error(F("Could not wake PN532"));
    // Arduino can't throw exceptions so we hack in a flag to detect wake failure
    _wake_success = false;
    return;
  }
}

bool PN532::wake_success(void) {
  return _wake_success;
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
  const uint8_t frame_length = (*count)+8;
  uint8_t i, checksum, frame[frame_length];
  _read_data(frame, frame_length);
  if (_debug) {
    Serial.print(F("Read frame: "));
    for (i=0; i<frame_length; i++) { _print_uint8_hex(frame[i]); }
    Serial.println();
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
  checksum = response_length + frame[offset+1]; // force to uint8_t
  if (checksum != 0) { return ERR_BAD_LENGTH_CHECKSUM; }
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
  offset += 1;
  response_length -= 1;
  // Return frame data
  if (response_length > (*count)) {
    // NB: this is necessary because the preamble and part of the start code may be missing
    return ERR_LONG_RESPONSE;
  }
  *count = response_length;
  for (i=0; i<response_length; i++) { response[i] = frame[offset+i]; }
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
    for (i=0; i<frame_length; i++) { _print_uint8_hex(frame[i]); }
    Serial.println();
  }
  _write_data(frame, frame_length);
}

// NB: Arduino doesn't support try/catch so we create a wrapper
uint8_t PN532::_call_function_try(uint8_t command, uint8_t * params, uint8_t params_len, uint8_t * response, uint8_t * response_len, uint16_t timeout) {
  uint8_t i, status, command_data[1+params_len], ack_p[ACK_LEN], response_data[1+(*response_len)];
  // write the command data to the PN532
  command_data[0] = command;
  for (i=0; i<params_len; i++) { command_data[i+1] = params[i]; }
  _write_frame(command_data, 1+params_len);
  // wait for ACK
  if (!_wait_ready(timeout)) {
    status = ERR_ACK_TIMEOUT;
    return status; // throw std::runtime_error(F("timed out waiting for command ACK"));
  }
  _read_data(ack_p, ACK_LEN);
  status = SUCCESS;
  for (i=0; i<ACK_LEN; i++) {
    if (ack_p[i] != ACK_FRAME[i]) { status = ERR_NO_COMMAND_ACK; }
  }
  if (status) { return status; } // throw std::runtime_error(F("no ACK after command"));
  // wait for response
  if (!_wait_ready(timeout)) {
    status = ERR_RESPONSE_TIMEOUT;
    return status; // throw std::runtime_error(F("timed out waiting for command response"));
  }
  *response_len += 1;
  status = _read_frame(response_data, response_len);
  if (status) { return status; } // throw std::runtime_error(F("bad command response frame"));
  if (response_data[0] != command+1) {
    status = ERR_WRONG_COMMAND_RESPONSE;
    return status; // throw std::runtime_error(F("wrong command response"));
  }
  // return the response
  *response_len -= 1;
  for (i=0; i<(*response_len); i++) { response[i] = response_data[i+1]; }
  return SUCCESS;
}

uint8_t PN532::_call_function(uint8_t command, uint8_t * params, uint8_t params_len, uint8_t * response, uint8_t * response_len, uint16_t timeout) {
  uint8_t status = _call_function_try(command, params, params_len, response, response_len, timeout);
  if (status) {
    // something went wrong while trying the command:
    // * send an ACK to abort (and reset communications)
    // * return the bad status code
    uint8_t ack_p[ACK_LEN];
    for (uint8_t i=0; i<ACK_LEN; i++) { ack_p[i] = ACK_FRAME[i]; }
    _write_data(ack_p, ACK_LEN);
  }
  return status;
}

bool PN532::get_firmware_version(uint32_t * version) {
  uint8_t version_length = 4;
  uint8_t status = _call_function(COMMAND_GETFIRMWAREVERSION, nullptr, 0, (uint8_t *)version, &version_length, 500);
  if (_debug && status) {
    Serial.print(F("COMMAND_GETFIRMWAREVERSION failed: "));
    _print_uint8_hex(status);
    Serial.print(F(" "));
    Serial.println(*version, HEX);
  }
  return (status == SUCCESS && version_length == 4);
}

bool PN532::SAM_disable(void) {
  // normal mode (SAM unused), no timeout (we use ACK to abort), and no IRQ
  uint8_t params[] = { 0x01, 0x00, 0x00 };
  uint8_t empty_length = 0;
  uint8_t status = _call_function(COMMAND_SAMCONFIGURATION, params, 3, nullptr, &empty_length, 500);
  if (_debug && status) {
    Serial.print(F("COMMAND_SAMCONFIGURATION failed: "));
    _print_uint8_hex(status);
    Serial.println();
  }
  return (status == SUCCESS);
}

// Largest response we care about
#define MAX_PAYLOAD_LEN (23)
bool PN532::read_passive_target_id(uint8_t * uid, uint8_t * uid_len, uint8_t card_baud_rate, uint16_t timeout) {
  uint8_t params[] = { 0x01, card_baud_rate };
  uint8_t r[MAX_PAYLOAD_LEN], r_len = MAX_PAYLOAD_LEN, r_uid_len, r_uid_offset, i;
  uint8_t status = _call_function(COMMAND_INLISTPASSIVETARGET, params, 2, r, &r_len, timeout);
  if (status || r_len <= 1 || r[0] == 0) { return false; }
  switch (card_baud_rate) {
    case ISO14443A_BAUD:
      if (r_len<6) { return false; }
      r_uid_len = r[5];
      r_uid_offset = 6;
      break;
    case FELICA_212_BAUD:
    case FELICA_424_BAUD:
      r_uid_len = 8;
      r_uid_offset = 4;
      break;
    case ISO14443B_BAUD:
      // Note: there are many flavors of ISO14443B cards.  We assume the most
      // common has an 8-byte UID in the attribute response.  However, some
      // cards do not store anything in the attribute response; some others
      // store an EPC of 24 bytes.  Perhaps it would be best to get the 4-byte
      // PUPI from the ATQB response (at offset 3).
      if (r_len<15) { return false; }
      r_uid_len = r[14];
      r_uid_offset = 15;
      break;
    case INNOVISION_JEWEL_BAUD:
      r_uid_len = 4;
      r_uid_offset = 4;
      break;
    default:
      // UNIMPLEMENTED
      return false;
  }
  // make sure we have enough data, and return the UID
  if (r_len<r_uid_offset+r_uid_len || (*uid_len)<r_uid_len) { return false; }
  *uid_len = r_uid_len;
  for (i=0; i<r_uid_len; i++) { uid[i] = r[r_uid_offset+i]; }
  return true;
}

bool PN532::power_down(void) {
  uint8_t params[] = { POWERDOWN_WAKEFROM_SPI, POWERDOWN_NO_IRQ };
  uint8_t powerdown_status, powerdown_status_len = 1;
  uint8_t status = _call_function(COMMAND_POWERDOWN, params, 2, &powerdown_status, &powerdown_status_len, 500);
  if (_debug) {
    if (status) {
      Serial.print(F("COMMAND_POWERDOWN failed: "));
      _print_uint8_hex(status);
      Serial.println();
    }
    else if (powerdown_status_len && powerdown_status) {
      Serial.print(F("COMMAND_POWERDOWN error: "));
      _print_uint8_hex(powerdown_status);
      Serial.println();
    }
  }
  if (status == SUCCESS && powerdown_status_len == 1 && powerdown_status == SUCCESS) {
    delay(POWERDOWN_DELAY);
    return true;
  }
  else {
    return false;
  }
}
