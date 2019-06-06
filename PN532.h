#ifndef PN532_H
#define PN532_H

#include <stdbool.h>
#include <stdint.h>

#include <Arduino.h>

#include "SPI_Device.h"

class PN532 {

public:

  // METHODS

  PN532(pin_size_t cs_pin, bool debug=false); // UNIMPLEMENTED: pin_size_t irq_pin
  bool get_firmware_version(uint32_t * version);
  bool SAM_disable(void);
  bool read_passive_target_id(uint8_t * uid, uint8_t * uid_len, uint8_t card_baud_rate=ISO14443A_BAUD, uint16_t timeout=1000);
  bool power_down(void);
  // UNIMPLEMENTED: ntag2xx_write_block(block_number, data) and ntag2xx_read_block(block_number)

private:

  // MEMBERS

  bool _debug;
  SPI_Device _spi_d;
  // UNIMPLEMENTED: pin_size_t _irq_pin;

  // METHODS

  void _wakeup(void);
  bool _wait_ready(uint16_t timeout=1000);
  void _read_data(uint8_t * data, uint8_t count);
  void _write_data(uint8_t * data, uint8_t count);
  uint8_t _read_frame(uint8_t * response, uint8_t * count);
  void _write_frame(uint8_t * data, uint8_t count);
  uint8_t _call_function(uint8_t command, uint8_t * params, uint8_t params_len, uint8_t * response, uint8_t * response_len, uint16_t timeout=1000);

  // CONSTANTS

  enum ERROR_CODES_E : uint8_t {
    SUCCESS                       = 0x00,
    // _read_frame
    ERR_BAD_PREAMBLE              = 0x01,
    ERR_BAD_PACKET_START          = 0x02,
    ERR_EMPTY_PACKET              = 0x03,
    ERR_EMPTY_RESPONSE            = 0x04,
    ERR_BAD_LENGTH_CHECKSUM       = 0x05,
    ERR_LONG_RESPONSE             = 0x06,
    ERR_BAD_DATA_CHECKSUM         = 0x07,
    ERR_BAD_TFI                   = 0x08,
    ERR_BAD_POSTAMBLE             = 0x09,
    // _call_function
    ERR_ACK_TIMEOUT               = 0x0A,
    ERR_NO_COMMAND_ACK            = 0x0B,
    ERR_RESPONSE_TIMEOUT          = 0x0C,
    ERR_WRONG_COMMAND_RESPONSE    = 0x0D
  };

  // frame markers
  enum FRAME_MARKERS_E : uint8_t {
    PREAMBLE                      = 0x00,
    STARTCODE1                    = 0x00,
    STARTCODE2                    = 0xFF,
    POSTAMBLE                     = 0x00
  };

  // frame identifier: to/from
  enum FRAME_IDENTIFIERS_E : uint8_t {
    HOSTTOPN532                   = 0xD4,
    PN532TOHOST                   = 0xD5
  };

  // special frames
  static const uint8_t ACK_LEN       = 6;
  static const uint8_t ACK_FRAME[]   = { 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00 };
  static const uint8_t NACK_FRAME[]  = { 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00 };
  static const uint8_t ERROR_FRAME[] = { 0x00, 0x00, 0xFF, 0x01, 0xFF, 0x7F, 0x81, 0x00 };

  // PN532 over SPI protocol: see section 6.2.5 of manual
  enum SPI_E : uint8_t {
    SPI_STATUSREAD                = 0x02,
    SPI_DATAWRITE                 = 0x01,
    SPI_DATAREAD                  = 0x03,
    SPI_READY                     = 0x01,
    SPI_STATUSPOLLINTERVAL        = 5 // milliseconds
  };

  // command set
  enum COMMANDS_E : uint8_t {
    COMMAND_DIAGNOSE              = 0x00,
    COMMAND_GETFIRMWAREVERSION    = 0x02,
    COMMAND_GETGENERALSTATUS      = 0x04,
    COMMAND_READREGISTER          = 0x06,
    COMMAND_WRITEREGISTER         = 0x08,
    COMMAND_READGPIO              = 0x0C,
    COMMAND_WRITEGPIO             = 0x0E,
    COMMAND_SETSERIALBAUDRATE     = 0x10,
    COMMAND_SETPARAMETERS         = 0x12,
    COMMAND_SAMCONFIGURATION      = 0x14,
    COMMAND_POWERDOWN             = 0x16,
    COMMAND_RFCONFIGURATION       = 0x32,
    COMMAND_INDATAEXCHANGE        = 0x40,
    COMMAND_INCOMMUNICATETHRU     = 0x42,
    COMMAND_INDESELECT            = 0x44,
    COMMAND_INJUMPFORPSL          = 0x46,
    COMMAND_INLISTPASSIVETARGET   = 0x4A,
    COMMAND_INPSL                 = 0x4E,
    COMMAND_INATR                 = 0x50,
    COMMAND_INRELEASE             = 0x52,
    COMMAND_INSELECT              = 0x54,
    COMMAND_INJUMPFORDEP          = 0x56,
    COMMAND_RFREGULATIONTEST      = 0x58,
    COMMAND_INAUTOPOLL            = 0x60,
    COMMAND_TGGETDATA             = 0x86,
    COMMAND_TGGETINITIATORCOMMAND = 0x88,
    COMMAND_TGGETTARGETSTATUS     = 0x8A,
    COMMAND_TGINITASTARGET        = 0x8C,
    COMMAND_TGSETDATA             = 0x8E,
    COMMAND_TGRESPONSETOINITIATOR = 0x90,
    COMMAND_TGSETGENERALBYTES     = 0x92,
    COMMAND_TGSETMETADATA         = 0x94
  };

  // constants for powering down and waking up
  enum POWERDOWN_E : uint8_t {
    POWERDOWN_WAKEFROM_SPI        = 0x20,
    POWERDOWN_NO_IRQ              = 0x00,
    POWERDOWN_DELAY               = 1, // milliseconds
    WAKE_DELAY                    = 2 // milliseconds (max), or 100µs min
  };

  // baud rate for InListPassiveTarget
  enum BAUD_E : uint8_t {
    // ISO/IEC 14443 Type A such as MIFARE and NTAG: 106 kbps
    ISO14443A_BAUD                = 0x00,
    // FeliCa polling: 212 and 424 kbps
    FELICA_212_BAUD               = 0x01,
    FELICA_424_BAUD               = 0x02,
    // ISO/IEC 14443-3 Type B: 106 kbps
    ISO14443B_BAUD                = 0x03,
    // Innovision Jewel tag: 106 kbps
    INNOVISION_JEWEL_BAUD         = 0x04
  };

}

#endif
