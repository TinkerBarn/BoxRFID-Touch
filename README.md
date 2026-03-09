# BoxRFID

BoxRFID is a standalone RFID tag tool for creating, reading and writing filament RFID tags for the **QIDI Box** ecosystem.

It runs on an **ESP32-2432S028R CYD (Cheap Yellow Display)** with a **PN532 NFC/RFID module** and provides a touchscreen-based interface for everyday tag handling without requiring a PC during normal use.

BoxRFID has been tested with:

- **QIDI Plus 4**
- **QIDI Box V1**
- **QIDI Box V2**

Compatibility with the **QIDI Q2** may be possible, but has not been tested yet. Feedback is welcome.

---

## Features

- Read RFID tags in **manual mode**
- Read RFID tags in **auto mode**
- Write RFID tags with:
  - **manufacturer**
  - **filament type**
  - **color**
- Standalone operation with only a **USB power source**
- Touchscreen user interface
- **6 UI languages**
- Customizable **manufacturer list**
- Customizable **material list**
- Reset custom lists to factory defaults
- Full factory reset including **touch calibration**
- Browser-based **Web Installer**
- Optional **3D printable housing**

- Integrated **BLE support** for use as an external RFID reader/writer for iOS devices
- Prepared for iOS companion app support (**app currently in alpha stage**)

---

## Hardware

### Supported hardware

- **ESP32-2432S028R CYD**
- **PN532 NFC/RFID module (I2C mode)**

### Required parts

- 1x ESP32-2432S028R CYD
- 1x PN532 NFC/RFID module
- jumper wires
- USB cable
- optional 3D printed enclosure

---

## Wiring

BoxRFID uses the PN532 in **I2C mode**.

### CYD to PN532

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
