#ifndef PN532_H
#define PN532_H

#include <stdbool.h>
#include <stdint.h>

#include "Pin.h"
#include "SPI_Interface.h"
#include "SPI_Device.h"

class PN532 {

public:

  // METHODS

  PN532(SPI_Interface spi_i, Pin cs_pin, bool debug=false); // UNIMPLEMENTED: Pin irq_pin
  uint32_t get_firmware_version(void);
  bool SAM_disable(void);
  bool read_passive_target_id(uint8_t * uid, uint8_t * uid_len, uint8_t card_baud_rate=ISO14443A_BAUD, uint16_t timeout=1000);
  // UNIMPLEMENTED: ntag2xx_write_block(block_number, data) and ntag2xx_read_block(block_number)

private:

  // MEMBERS

  bool _debug;
  SPI_Device _spi_d;
  // UNIMPLEMENTED: Pin _irq_pin;

  // METHODS

  bool _call_function(uint8_t command, uint8_t * params, uint8_t params_len, uint8_t * response, uint8_t * response_len, uint8_t response_len_limit=0, uint16_t timeout=1000);
  void _wakeup(void);
  bool _wait_ready(uint16_t timeout=1000);
  void _read_data(uint8_t * data, uint8_t count);
  void _write_data(uint8_t * data, uint8_t count);
  bool _read_frame(uint8_t * response, uint8_t * count);
  void _write_frame(uint8_t * data, uint8_t count);

  // CONSTANTS

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
  static const uint8_t ACK_FRAME[]   = { 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00 };
  static const uint8_t NACK_FRAME[]  = { 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00 };
  static const uint8_t ERROR_FRAME[] = { 0x00, 0x00, 0xFF, 0x01, 0xFF, 0x7F, 0x81, 0x00 };

  // SPI protocol
  enum SPI_E : uint8_t {
    SPI_STATREAD                  = 0x02,
    SPI_DATAWRITE                 = 0x01,
    SPI_DATAREAD                  = 0x03,
    SPI_READY                     = 0x01
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

// FIXME: not necessary because the proper response code is always command instruction +1?
  // command response codes
  enum RESPONSES_E : uint8_t {
    RESPONSE_INDATAEXCHANGE       = 0x41,
    RESPONSE_INLISTPASSIVETARGET  = 0x4B
  };

  // constants for powering down and waking up
  enum POWERDOWN_E : uint8_t {
    POWERDOWN_WAKEFROM_SPI        = 0x20,
    // dummy byte to send when waiting out the wakeup delay from exiting power down mode
    WAKEUP_BYTE                   = 0x55,
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
