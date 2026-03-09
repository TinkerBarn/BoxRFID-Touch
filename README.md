# BoxRFID-Touch – Standalone RFID Tag Tool for QIDI Box

BoxRFID-Touch is a standalone touchscreen device for reading and writing NFC/RFID filament tags used by **QIDI Box**. It runs on an **ESP32-2432S028R CYD** with a **PN532** reader and lets you create or modify tag data directly on the device — no PC required during normal use.

- Platform: ESP32-2432S028R CYD + PN532 (I2C)
- Firmware: Version 2.0
- License: CC BY-NC-SA 4.0

## Features

- Read RFID tags in **manual mode**
- Read RFID tags in **auto mode**
- Write filament data to compatible tags:
  - **manufacturer**
  - **filament type**
  - **color**
- Standalone operation with only **USB power**
- Touchscreen-based user interface
- Multi language support (**6 languages**)
- Support for customized **manufacturer list**
- Support for customized **material list**
- Reset custom lists back to factory defaults
- Full factory reset including **touch calibration**
- Integrated **BLE support**
- Can be used as an external RFID reader/writer via Bluetooth
- Intended for iOS companion app usage (**app currently in alpha stage**)
- Browser-based **Web Installer** for quick installation
- Optional **3D printable housing**

## Compatibility

Tested with:

- **QIDI Plus 4**
- **QIDI Box V1**
- **QIDI Box V2**

Possible compatibility with:

- **QIDI Q2**  
  Not tested yet. Feedback is welcome.

## WEB INSTALLER – QUICK START!

> **IMPORTANT:**  
> You do **not** need to compile the firmware yourself!  
> You can install BoxRFID-Touch directly from your browser using the Web Installer:
>
> **[BoxRFID-Touch – Open Web Installer](https://tinkerbarn.github.io/BoxRFID-Touch/)**
>
> This is the recommended installation method for most users.

### Requirements

- **Chrome** or **Edge**
- supported **ESP32-2432S028R CYD**
- USB **data cable**
- connected **PN532** module

### Why this is easy

The Web Installer is intended especially for users who do not want to work with:

- Arduino IDE
- library installation
- ESP32 board package setup
- TFT_eSPI configuration

Just connect the device, open the installer page, click **Connect**, select the serial port and flash the firmware.

### Notes

- If the board is not detected immediately, reconnect it and try again
- If needed, keep the **BOOT** button pressed while connecting
- During normal use, the device only needs a **USB power source**

## REQUIREMENTS

### Main hardware

- **ESP32-2432S028R CYD**
- **PN532 NFC/RFID module**
- **USB cable**
- **jumper wires**

### Optional

- **3D printed enclosure**
- mounting hardware depending on enclosure design

## BOM

### Electronics

- **ESP32-2432S028R CYD**
  - Main controller with integrated display and touchscreen
  - [Amazon Germany ESP32-2432S028R](https://www.amazon.de/dp/B0CG2WQGP9)
  - ASIN: B0CG2WQGP9

- **PN532 NFC/RFID module**
  - Used for reading and writing compatible RFID/NFC tags
  - [Amazon Germany PN532](https://www.amazon.de/dp/B0D86CPN5J)
  - ASIN: B0D86CPN5

- **Jumper wires**
  - Used to connect CYD and PN532 (included with ESP32-2432S028R)

- **USB cable**
  - Used for flashing and power supply

### Optional parts

- **3D printed case**
  - Housing for the CYD + PN532 setup

- **Mounting material**
  - Screws, spacers, adhesive pads or similar

## WIRING

BoxRFID-Touch uses the PN532 in **I2C mode**.

### CYD ↔ PN532

- **3.3V** → **VCC**
- **GND** → **GND**
- **GPIO 21** → **SDA**
- **GPIO 22** → **SCL**

```text
ESP32-2432S028R CYD    PN532
-------------------    -----
3.3V                -> VCC
GND                 -> GND
GPIO 21             -> SDA
GPIO 22             -> SCL
