#include "PN532.h"

#define PN532_CS_PIN (5)

PN532 * the_pn532;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  while (!Serial);
  SPI.begin();
  the_pn532 = new PN532(PN532_CS_PIN, true);
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
  while (1);
}

// Arduino doesn't have this, but one can wish ...
void teardown() {
  delete the_pn532;
  SPI.end();
  Serial.end();
}
