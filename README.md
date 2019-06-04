# nfc-ble-keyboard
Code for the Adafruit Feather M0 Bluefruit and PN532 breakout to act as a BLE keyboard that reads NTAG serial numbers.

# Credits

This project was inspired by code from [Adafruit](https://www.adafruit.com).  In particular, the following libraries:
* https://github.com/adafruit/Adafruit_Blinka
* https://github.com/adafruit/Adafruit_BluefruitLE_nRF51
* https://github.com/adafruit/Adafruit_CircuitPython_BusDevice
* https://github.com/adafruit/Adafruit_CircuitPython_PN532
* https://github.com/adafruit/Adafruit-PN532
* https://github.com/adafruit/circuitpython (itself a fork of MicroPython)

For interfacing with the PN532 for NFC, the CircuitPython libraries are more up-to-date and better organized, but I am currently using the Feather M0 Bluefruit which doesn't have the storage space for the interpreter.  On the other hand, the CircuitPython BLE libraries are focussed on the Bluefruit UART custom protocol, and HID device emulation examples are limited to USB; the Arduino libraries, meanwhile, have ready-made examples.
