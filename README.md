# BoxRFID

BoxRFID is a standalone RFID/NFC-based filament tag tool for the **QIDI Box** ecosystem.

It runs on an **ESP32-2432S028R CYD (Cheap Yellow Display)** together with a **PN532 NFC/RFID reader** and provides a touchscreen-based interface for reading, creating and writing filament RFID tags directly on the device.

BoxRFID is intended as an easy-to-use hardware solution for makers who want to work with RFID filament tags without needing a PC during normal operation.

It has been tested with:

- **QIDI Plus 4**
- **QIDI Box V1**
- **QIDI Box V2**

Compatibility with the **QIDI Q2** may be possible, but has not been tested yet. Feedback is welcome.

---

## What is BoxRFID?

BoxRFID is a compact standalone device for working with RFID/NFC filament tags used in the QIDI Box ecosystem.

It is designed for practical day-to-day use and allows you to handle common RFID tag tasks directly on the touchscreen device instead of relying only on software tools or a permanently connected computer.

Typical use cases include:

- reading existing RFID filament tags
- creating new tags for QIDI Box use
- writing manufacturer, material and color information
- managing spool-related tag data in a simple dedicated workflow

---

## Features

- Read RFID tags in **manual mode**
- Read RFID tags in **auto mode**
- Write RFID tags with:
  - **manufacturer**
  - **filament type**
  - **color**
- Touchscreen-based standalone operation
- Only a **USB power source** is required during normal use
- **6 user interface languages**
- Customizable **manufacturer list**
- Customizable **material list**
- Reset custom lists to factory defaults
- Full factory reset including **touch calibration**
- Integrated **BLE support**
- Can be used as an external RFID reader/writer via Bluetooth
- Intended for iOS companion app usage
- Browser-based **Web Installer** for easy setup
- Optional **3D printable housing**

---

## Why the Web Installer?

BoxRFID can be installed using a browser-based **Web Installer**, making first-time setup much easier for users who do not want to work with the Arduino IDE, install libraries manually or configure display files themselves.

This means that even beginners can flash the firmware quickly with only a few steps:

1. connect the supported ESP32 board via USB
2. open the Web Installer page
3. click **Connect**
4. select the board
5. install the firmware

For normal users, this is the easiest and fastest way to get started with BoxRFID.

Advanced users can still build the firmware manually from source if they want to modify or extend the project.

---

## Supported Hardware

Currently supported:

- **ESP32-2432S028R CYD**
- **PN532 NFC/RFID module (I2C mode)**

At the moment, the project is focused on one supported hardware variant to keep installation and usage simple.

---

## Web Installer

You can install the firmware directly from your browser using the BoxRFID Web Installer:

**[Open Web Installer](https://YOURNAME.github.io/BoxRFID-WebInstaller/)**

### Requirements

- **Chrome** or **Edge**
- USB data cable
- supported ESP32 board

### Notes

- If needed, keep the **BOOT** button pressed while connecting
- The Web Installer is intended to provide a simple installation path for users without Arduino knowledge
- During normal use, BoxRFID only needs USB power and does not require a PC connection

---

## Hardware Required

### Main Components

- 1x **ESP32-2432S028R CYD** display board
- 1x **PN532 NFC/RFID module**
- 1x suitable **USB cable with data support**
- jumper wires
- optional 3D printed case

---

## BOM

### Electronics

- **ESP32-2432S028R CYD**
  - Main controller with integrated touchscreen display

- **PN532 RFID/NFC module**
  - Used for reading and writing RFID/NFC tags

- **Jumper wires**
  - Used to connect CYD and PN532

- **USB cable**
  - Used for flashing and power supply

### Optional Parts

- **3D printed enclosure**
  - Custom housing for the CYD + PN532 setup

- **Mounting material**
  - Screws, spacers, adhesive pads or similar depending on the enclosure design

---

## Wiring

BoxRFID uses the PN532 in **I2C mode**.

### CYD to PN532 wiring

- **3.3V** → **VCC**
- **GND** → **GND**
- **GPIO 21** → **SDA**
- **GPIO 22** → **SCL**

### Wiring Overview

```text
ESP32-2432S028R CYD    PN532
-------------------    -----
3.3V                -> VCC
GND                 -> GND
GPIO 21             -> SDA
GPIO 22             -> SCL
