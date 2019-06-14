#include "PN532.h"

#define PN532_CS_PIN (5)

PN532 * the_pn532;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  while (!Serial);
  SPI.begin();
  the_pn532 = new PN532(PN532_CS_PIN);
  if (!the_pn532->wake_success()) {
    // the PN532 either wasn't detected, or it won't wake up
    Serial.println(F("Couldn't find the PN532.  Make sure to check the wiring."));
    while (1);
  }
  // Print the PN532 version info
  uint32_t version;
  the_pn532->get_firmware_version(&version);
  Serial.println(version, HEX); // 0x07060132 in reverse order
  // Disable the SAM
  the_pn532->SAM_disable();
}
 
// the loop function runs over and over again forever
void loop() {
  uint8_t i, uid[10], uid_len = 10;
  bool success;
  success = the_pn532->read_passive_target_id(uid, &uid_len); // ISO14443A_BAUD, 100
  if (success) {
    Serial.print(F("got tag ID: "));
    for (i=0; i<uid_len; i++) { _print_uint8_hex(uid[i]); }
    Serial.print(F(" "));
    Serial.print(uid_len, DEC);
    Serial.println();
    delay(3000); // wait to avoid inadvertently scanning the same tag twice
  }
  else {
    Serial.println(F("Waiting for tag ..."));
    delay(100);
  }
}

// Arduino doesn't have this, but one can wish ...
void teardown() {
  delete the_pn532;
  SPI.end();
  Serial.end();
}
