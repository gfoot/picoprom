PicoPROM - DIP-EEPROM Programmer based on Raspberry Pi Pico
===========================================================

Writes ROM images to parallel-interface EEPROMs via XMODEM file transfer over USB.

Warning
-------
The Raspberry Pi Pico is not 5V-tolerant, but these EEPROMs are 5V devices.
This is fine for write-only access, but it's important to ensure the EEPROM
never writes back to the Pico's GPIO pins.  Make sure OE is permanently wired
high to prevent this.

Features
--------

* No additional components are required - just the Pico and the EEPROM
* XMODEM+CRC transfer protocol makes it easy to send files to be written to the EEPROM
* Fast operation - comparable to TL866
* Supports paged writes as well as byte-writes
* Supports write protection

Not features
------------
* Reading back and verifying is not supported and probably won't ever be
* Configuration can't be changed at runtime yet, it needs to be set in code
* YMODEM, ZMODEM, and other XMODEM extensions aren't supported yet

Binary Installation
-------------------
1. Hold BOOTSEL and connect the Raspberry Pi Pico
2. Drag picoprom.uf2 into the Raspberry Pi Pico's mass storage window
3. It should then reboot and start communicating over USB Serial

Usage
-----
1. Wire the Raspberry Pi Pico up to the EEPROM according to the pinout table below
2. Connect the Raspberry Pi Pico to the computer by USB
3. Launch a terminal app with XMODEM support, such as Tera Term, and connect it to the Raspberry Pi Pico
4. Verify that the Raspberry Pi Pico is reporting that it's ready to receive a ROM image - it will generally print a lot of letter C characters if it's ready
5. Use your terminal to send a ROM image using the XMODEM+CRC protocol

Pinout
------
The pinout is quite straightforward.

| Pico pin | EEPROM pin | Function |
| -------- |:----------:| -------- |
| GP2      | 26 | A13    |
| GP3      | 1  | A14    |
| GP4      | 2  | A12    |
| GP5      | 3  | A7     |
| GP6      | 4  | A6     |
| GP7      | 5  | A5     |
| GP8      | 6  | A4     |
| GP9      | 7  | A3     |
| GP10     | 8  | A2     |
| GP11     | 9  | A1     |
| GP12     | 10 | A0     |
| GP13     | 11 | D0     |
| GP14     | 12 | D1     |
| GP15     | 13 | D2     |
|          |    |        |
| GP16     | 15 | D3     |
| GP17     | 16 | D4     |
| GP18     | 17 | D5     |
| GP19     | 18 | D6     |
| GP20     | 19 | D7     |
| GP21     | 20 | CE     |
| GP22     | 21 | A10    |
| GP26     | 23 | A11    |
| GP27     | 24 | A9     |
| GP28     | 25 | A8     |
|          |    |        |
| VBUS     | 28 | VCC    |
| VBUS     | 22 | OE     |
| GND      | 14 | GND    |
| GND      | 27 | WE     |

Cloning and Building from Source
--------------------------------
1. Set up `pico-sdk` according to the Pico's getting-started guide.
2. Clone this repository alongside `pico-sdk`
3. Create a `build` folder
4. From within the `build` folder, type: `cmake ..`
5. From within the `build` folder, type: `make`

