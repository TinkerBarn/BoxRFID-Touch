# BoxRFID-Touch

Standalone RFID/NFC touchscreen tool for **QIDI Box** and **OpenSpool** workflows on **ESP32-2432S028R (CYD)** with **PN532**.

<p align="center">
  <a href="https://tinkerbarn.github.io/BoxRFID-Touch/">
    <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/Front.jpeg" height="300" alt="BoxRFID-Touch device">
  </a>
</p>

## Web Installer

> The fastest way to get started. No Arduino IDE, no library setup, no TFT_eSPI configuration.

## [Open BoxRFID-Touch Web Installer](https://tinkerbarn.github.io/BoxRFID-Touch/)

This is the recommended installation method for most users.

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/H2H41XBKJ6)

---

## Firmware Overview

| Firmware line | Version | Status | Use case | Installer |
| --- | --- | --- | --- | --- |
| BoxRFID-Touch | V2.1 | Stable | Classic QIDI workflow with MIFARE Classic 1K tags | Yes |
| BoxRFID OpenSpool Edition | V3.5 | Stable | QIDI plus OpenSpool plus Snapmaker U1 OpenRFID workflows | Yes |
| BoxRFID-Touch | V2.0 | Legacy | Older classic QIDI release kept for compatibility | No |

Public firmware binaries in this repository:

- [BoxRFID-Touch V2.0](./firmware/boxrfid-touch/v2.0/)
- [BoxRFID-Touch V2.1](./firmware/boxrfid-touch/v2.1/)
- [BoxRFID OpenSpool Edition V3.5](./firmware/boxrfid-openspool/v3.5/)

Detailed references:

- [Firmware Matrix](./docs/firmware-matrix.md)
- [BoxRFID-Touch documentation](./docs/BoxRFID-Touch/BoxRFID-Touch.md)
- [BoxRFID OpenSpool Edition documentation](./docs/BoxRFID-OpenSpool-Edition/BoxRFID-OpenSpool-Edition.md)

---

## What It Does

BoxRFID-Touch lets you read and write compatible filament RFID/NFC tags directly on the device without needing a PC during normal use.

Main capabilities:

- Read RFID tags in manual mode
- Read RFID tags in auto mode
- Write manufacturer, filament type, and color to compatible tags
- Use a touchscreen-based standalone interface
- Run on USB power only during normal operation
- Use multilingual firmware with customizable material and manufacturer lists

Additional OpenSpool Edition capabilities:

- Read and write OpenSpool standard tags
- Support Snapmaker U1 OpenRFID workflows
- Use expanded OpenSpool-related setup and writing options

---

## Which Version Should I Use

- Choose **BoxRFID-Touch V2.1** if you only need the classic QIDI workflow.
- Choose **BoxRFID OpenSpool Edition V3.5** if you need OpenSpool support or Snapmaker U1 OpenRFID compatibility.
- Use **BoxRFID-Touch V2.0** only if you specifically need the older classic firmware.

---

## Installation

### Web Installer

1. Connect the ESP32 board via USB.
2. Open the [Web Installer](https://tinkerbarn.github.io/BoxRFID-Touch/).
3. Click **Connect**.
4. Select the correct serial port.
5. Choose the desired firmware.
6. Flash the firmware.

Notes:

- Use **Chrome** or **Edge**
- Use a **data-capable USB cable**
- Reconnect the board if it is not detected immediately
- If needed, hold the **BOOT** button while connecting
- Flashing will overwrite the existing firmware on the device

---

## Hardware

Main hardware:

- **ESP32-2432S028R CYD**
- **PN532 NFC/RFID module**
- **USB cable**
- **Jumper wires**

Supported tag types:

- **QIDI workflow:** MIFARE Classic 1K tags
- **OpenSpool workflow:** NTAG215 tags

---

## Compatibility

### BoxRFID-Touch

Tested with:

- **QIDI Plus 4**
- **QIDI Box V1**
- **QIDI Box V2**

Possible compatibility:

- **QIDI Q2** not tested yet

### BoxRFID OpenSpool Edition

Designed for:

- **OpenSpool standard tags**
- **Snapmaker U1** with **paxx12 extended firmware**
- **Snapmaker U1** with **OpenRFID support**

---

## Wiring

Set the PN532 module to **I2C mode** before use.

```text
ESP32-2432S028R CYD    PN532
-------------------    -----
3.3V                -> VCC
GND                 -> GND
GPIO 27             -> SDA
GPIO 22             -> SCL
```

---

## Video

[![Watch the video](https://img.youtube.com/vi/4cGLlr9Ckx4/hqdefault.jpg)](https://youtu.be/4cGLlr9Ckx4?is=iYzOwJqUVbCeVkuv)

---

## Bill Of Materials

Electronics:

- **ESP32-2432S028R CYD**
  [Amazon Germany](https://www.amazon.de/dp/B0CG2WQGP9)
  [Amazon USA](https://www.amazon.com/dp/B0DNM4SKSJ)
- **PN532 NFC/RFID module**
  [Amazon Germany](https://www.amazon.de/dp/B0D86CPN5J)
  [Amazon USA](https://www.amazon.com/dp/B01I1J17LC)
- **Female to Female USB-C Data Cable**
  [Amazon Germany](https://www.amazon.de/dp/B0DSLNJMDR)
  [Amazon USA](https://www.amazon.com/dp/B0C1X7P9K2)
- **USB-C switch**
  [Amazon Germany](https://www.amazon.de/dp/B0CG11Y3MD)
  [Amazon USA](https://www.amazon.com/dp/B0F23RKY9Z)
- **Jumper wires**

Case:

- [BoxRFID-Touch case on MakerWorld](https://makerworld.com/de/models/2518866-boxrfid-case-rfid-reader-writer-for-qidi-box#profileId-2770921)

---

## Photos And Screenshots

### Hardware

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

### Assembly

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

### Device

<table align="center">
  <tr>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Front.jpeg" height="180" alt="Front view"><br>
      <sub>Front view</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Back.jpeg" height="180" alt="Back view"><br>
      <sub>Back view</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Side.jpeg" height="180" alt="Side view"><br>
      <sub>Side view</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Mounted.jpeg" height="180" alt="Mounted device"><br>
      <sub>Mounted device</sub>
    </td>
  </tr>
</table>

### UI Screenshots

<table align="center">
  <tr>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/HomeDetailed.jpeg" height="180" alt="Home"><br>
      <sub>Home</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/ReadTag.jpeg" height="180" alt="Read Tag"><br>
      <sub>Read Tag</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/WriteMain.jpeg" height="180" alt="Write Main"><br>
      <sub>Write Main</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/SetupMain.jpeg" height="180" alt="Setup Main"><br>
      <sub>Setup Main</sub>
    </td>
  </tr>
</table>

---

## Related Project

If you need a Windows desktop program, see [BoxRFID](https://github.com/TinkerBarn/BoxRFID).

---

## License

CC BY-NC-SA 4.0
