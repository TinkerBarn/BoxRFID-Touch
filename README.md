# BoxRFID-Touch – Standalone RFID Tag Tool for QIDI Box

<p align="center">
  <a href="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Home.jpeg">
    <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/Front.jpeg" height="320" alt="BoxRFID-Touch device">
  </a>
</p>

BoxRFID-Touch is a standalone touchscreen device for reading and writing NFC/RFID filament tags used by the **QIDI Box** ecosystem.  
It runs on an **ESP32-2432S028R CYD** together with a **PN532** RFID/NFC reader and allows you to create, read and modify tag data directly on the device — no PC required during normal use.

In addition to the classic **BoxRFID-Touch** firmware, the project now also includes **BoxRFID OpenSpool Edition**, a significantly extended firmware variant with support for **OpenSpool standard tags** and dedicated support for the **Snapmaker U1** running **paxx12 extended firmware** with **OpenRFID support**.

For detailed feature overviews, see:
- [BoxRFID OpenSpool Edition documentation](https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/docs/BoxRFID-OpenSpool-Edition/BoxRFID-OpenSpool-Edition.md)
- [BoxRFID-Touch documentation](https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/docs/BoxRFID-Touch/BoxRFID-Touch.md)

- **Platform:** ESP32-2432S028R CYD + PN532 (I2C)
- **Firmware:** BoxRFID-Touch V2.1 and BoxRFID OpenSpool Edition V3.5
- **License:** CC BY-NC-SA 4.0

---
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/H2H41XBKJ6)

---

## 🚀 WEB INSTALLER – THE EASIEST WAY TO GET STARTED

> **No Arduino IDE. No library setup. No TFT_eSPI configuration.**
>
> BoxRFID-Touch can be installed directly from your browser in just a few clicks:
>
> ## **[➡ Open BoxRFID-Touch Web Installer](https://tinkerbarn.github.io/BoxRFID-Touch/)**

This is the **recommended installation method for most users**, especially beginners.

### What you need

- **Chrome** or **Edge**
- a supported **ESP32-2432S028R CYD**
- a **USB data cable**
- a connected **PN532** module

### Why the Web Installer is easier

The Web Installer is designed for users who do **not** want to deal with:

- Arduino IDE
- manual library installation
- ESP32 board package setup
- TFT_eSPI configuration

### Installation steps

1. Connect the ESP32 board via USB
2. Open the Web Installer
3. Click **Connect**
4. Select the correct serial port
5. Choose the desired firmware variant
6. Flash the firmware
7. Start using BoxRFID-Touch

### Notes

- If the board is not detected immediately, reconnect it and try again
- If needed, keep the **BOOT** button pressed while connecting
- During normal use, the device only needs a **USB power source**

---

## Features

### BoxRFID-Touch

- Read RFID tags in **manual mode**
- Read RFID tags in **auto mode**
- Write filament data to compatible tags:
  - **manufacturer**
  - **filament type**
  - **color**
- Standalone operation with only **USB power**
- Touchscreen-based user interface
- Multi-language support (**6 languages**)
- Support for customized **manufacturer lists**
- Support for customized **material lists**
- Reset custom lists back to factory defaults
- Full factory reset including **touch calibration**
- Integrated **BLE support**
- Can be used as an external RFID reader/writer via Bluetooth
- Intended for iOS companion app usage (**app currently in alpha stage**)
- Browser-based **Web Installer** for quick and easy installation
- Optional **3D-printable housing**

### BoxRFID OpenSpool Edition

- Significantly extended firmware variant based on the BoxRFID platform
- Support for **OpenSpool standard tags**
- Create and write OpenSpool tags with **manufacturer**, **material**, and **color**
- Dedicated support for **Snapmaker U1**
- Compatible with **paxx12 extended firmware**
- Supports **OpenRFID-enabled** Snapmaker U1 workflows
- Extended user interface and setup options compared to the classic BoxRFID-Touch firmware

### Detailed Feature Documentation

- [BoxRFID OpenSpool Edition – detailed feature description](https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/docs/BoxRFID-OpenSpool-Edition/BoxRFID-OpenSpool-Edition.md)
- [BoxRFID-Touch – detailed feature description](https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/docs/BoxRFID-Touch/BoxRFID-Touch.md)

### See details on YouTube
[![Watch the video](https://img.youtube.com/vi/4cGLlr9Ckx4/hqdefault.jpg)](https://youtu.be/4cGLlr9Ckx4?is=iYzOwJqUVbCeVkuv)

---

## Compatibility

### BoxRFID-Touch

Tested with:

- **QIDI Plus 4**
- **QIDI Box V1**
- **QIDI Box V2**

Possible compatibility with:

- **QIDI Q2**  
  Not tested yet. Feedback is welcome.

### BoxRFID OpenSpool Edition

Designed for:

- **OpenSpool standard tags**
- **Snapmaker U1** with **paxx12 extended firmware**
- **Snapmaker U1** with enabled **OpenRFID support**

### If you need a Windows program, see my other project
https://github.com/TinkerBarn/BoxRFID

---

## Requirements

### Main hardware

- **ESP32-2432S028R CYD**
- **PN532 NFC/RFID module**
- **USB cable**
- **jumper wires**

### RFID Tags

- **QIDI RFID tags: Mifare classic 1K tags**
  - For the classic BoxRFID-Touch / QIDI workflow
  - i.e.:
    - https://www.amazon.de/dp/B07GH1P2M5
    - https://www.abcrfid.com/product/13-56mhz-mifare-classic-1ks50-wet-inlay-thin-tag-adhesive-sticker
    - https://www.amazon.com/dp/B081CFXCBJ

- **OpenSpool tags: NTAG215 tags**
  - For BoxRFID OpenSpool Edition / OpenSpool workflows
  - i.e.:
    - https://www.amazon.de//dp/B0GJ4T9GG2
    - https://www.amazon.com/dp/B081CFXCBJ

### BoxRFID-Touch Case

- **3D-printed case**
  - Housing for the CYD + PN532 setup
  - ### [Box-RFID-Touch on Makerworld](https://makerworld.com/de/models/2518866-boxrfid-case-rfid-reader-writer-for-qidi-box#profileId-2770921)

---

## BOM

### Electronics

- **ESP32-2432S028R CYD**
  - 2.8 inch ESP32-2432S028R ESP32 Yellow Display - 9341 Resistive Touchscreen 240x320 TFT LCD Module CYD
  - [Amazon Germany ESP32-2432S028R](https://www.amazon.de/dp/B0CG2WQGP9)
    - ASIN: B0CG2WQGP9
  - [Amazon USA ESP32-2432S028R](https://www.amazon.com/dp/B0DNM4SKSJ)
    - ASIN: B0DNM4SKSJ


- **PN532 NFC/RFID module**
  - Used for reading and writing compatible RFID/NFC tags
  - [Amazon Germany PN532](https://www.amazon.de/dp/B0D86CPN5J)
    - ASIN: B0D86CPN5J
  - [Amazon USA PN532](https://www.amazon.com/dp/B01I1J17LC)
    - ASIN: B01I1J17LC

   
- **Female to Female USB-C Data Cable**
  - Used for flashing and power supply
  - [Amazon Germany USB-C cable](https://www.amazon.de/dp/B0DSLNJMDR)
    - ASIN: B0DSLNJMDR
  - [Amazon USA USB-C cable](https://www.amazon.com/dp/B0C1X7P9K2)
    - ASIN: B0C1X7P9K2


- **USB-C switch**
  - External switch to power on/off
  - [Amazon Germany USB-C switch](https://www.amazon.de/dp/B0CG11Y3MD)
    - ASIN: B0CG11Y3MD
  - [Amazon USA USB-C switch](https://www.amazon.com/dp/B0F23RKY9Z)
    - ASIN: B0F23RKY9Z

    
- **Jumper wires**
  - Used to connect the CYD and PN532  
  - Often included with the ESP32-2432S028R board


- **Mounting material**
  - Screws, spacers, adhesive pads or similar accessories

---

## Wiring

BoxRFID-Touch uses the PN532 RFID module in **I2C mode**.  
Do not forget to switch the PN532 to **I2C mode** before use. Depending on your module version, this is usually done with the DIP switches or jumpers. See the detailed photos below.

The following pinout shows how to connect the PN532 RFID board to the ESP32-2432S028R.

### Pinout CYD ↔ PN532
```text
ESP32-2432S028R CYD    PN532
-------------------    -----
3.3V                -> VCC
GND                 -> GND
GPIO 27             -> SDA
GPIO 22             -> SCL
```

### Overview of Necessary Parts

<table align="center">
  <tr>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Electronic%20parts.jpeg" height="180" alt="Electronic parts"><br>
      <sub>Electronic parts</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/PN532.jpeg" height="180" alt="PN532 RFID sensor"><br>
      <sub>PN532 RFID sensor</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/PN532-I2C.jpeg" height="180" alt="Set PN532 to I2C mode"><br>
      <sub>Set PN532 to I2C mode</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/ESP32-2432S028R.jpeg" height="180" alt="ESP32-2432S028R"><br>
      <sub>ESP32-2432S028R</sub>
    </td>
  </tr>
</table>

### Details on How to Connect the Boards

<table align="center">
  <tr>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Cable%20connection.jpeg" height="180" alt="Connect the cables"><br>
      <sub>Connect the cables</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Detail%20cable.jpeg" height="180" alt="ESP32 detail view"><br>
      <sub>ESP32 detail view</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Detail%20PN532.jpeg" height="180" alt="PN532 detail view"><br>
      <sub>PN532 detail view</sub>
    </td>
  </tr>
</table>

### Case Mounting

<table align="center">
  <tr>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Place%20PN532.jpeg" height="180" alt="Place PN532"><br>
      <sub>Place PN532</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Place%20parts.jpeg" height="180" alt="Place ESP32 in case"><br>
      <sub>Place ESP32 in case</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Mount%20CYD.jpeg" height="180" alt="Mount CYD with M3 6 mm screws"><br>
      <sub>Mount CYD with M3 6 mm screws</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Mounted.jpeg" height="180" alt="All parts mounted"><br>
      <sub>All parts mounted</sub>
    </td>
  </tr>
  <tr>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Place%20PN532.jpeg" height="180" alt="Base plate and adhesive pads"><br>
      <sub>Base plate and adhesive pads</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Base%20plate%20with%20feeds.jpeg" height="180" alt="Base plate with mounted pads"><br>
      <sub>Base plate with mounted pads</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Close%20Case.jpeg" height="180" alt="Close the case"><br>
      <sub>Close the case</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Screw01.jpeg" height="180" alt="M3 5 mm countersunk screw"><br>
      <sub>M3 5 mm countersunk screw</sub>
    </td>
  </tr>
  <tr>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Screw02.jpeg" height="180" alt="Bolted base plate"><br>
      <sub>Bolted base plate</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Side.jpeg" height="180" alt="Side view"><br>
      <sub>Side view</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Front.jpeg" height="180" alt="Front view"><br>
      <sub>Front view</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Back.jpeg" height="180" alt="Back view"><br>
      <sub>Back view</sub>
    </td>
  </tr>
</table>

## Screenshots

<table align="center">
  <tr>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/HomeDetailed.jpeg" height="180" alt="Home detailed"><br>
      <sub>Home</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/ReadTag.jpeg" height="180" alt="Read tag"><br>
      <sub>Read Tag</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/ReadManually.jpeg" height="180" alt="Read manually"><br>
      <sub>Read Manually</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/WriteMain.jpeg" height="180" alt="Write main"><br>
      <sub>Write Main</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/WriteMaterial.jpeg" height="180" alt="Write material"><br>
      <sub>Write Material</sub>
    </td>
  </tr>
  <tr>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/WriteColor.jpeg" height="180" alt="Write color"><br>
      <sub>Write Color</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/SetupMain.jpeg" height="180" alt="Setup main"><br>
      <sub>Setup Main</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/SetupLanguage.jpeg" height="180" alt="Setup language"><br>
      <sub>Setup Language</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/SetupManufacturer.jpeg" height="180" alt="Setup manufacturer"><br>
      <sub>Setup Manufacturer</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/SetupMaterial.jpeg" height="180" alt="Setup material"><br>
      <sub>Setup Material</sub>
    </td>
  </tr>
</table>
