# BoxRFID-Touch

Standalone RFID/NFC touchscreen tool for **QIDI** and **OpenSpool** workflows on **ESP32-2432S028R (CYD)** with **PN532**.

<p align="center">
  <a href="https://tinkerbarn.github.io/BoxRFID-Touch/">
    <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/Front.jpeg" height="300" alt="BoxRFID-Touch device">
  </a>
</p>

## Web Installer

> The fastest way to get started. No Arduino IDE, no library setup, no TFT_eSPI configuration.

## [Open BoxRFID-Touch Web Installer](https://tinkerbarn.github.io/BoxRFID-Touch/)

The web installer now offers three clear choices:

- **V2.1 Classic QIDI** for the classic firmware line
- **V3.7 Fallback** for the previous combined QIDI + OpenSpool release
- **V4.0 Current Release** for the latest combined firmware

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/H2H41XBKJ6)

---

## Video

[![Watch the video](https://img.youtube.com/vi/4cGLlr9Ckx4/hqdefault.jpg)](https://youtu.be/4cGLlr9Ckx4?is=iYzOwJqUVbCeVkuv)

> Note: This video shows an older workflow and does not yet reflect the current V4.0 release.

---

## Firmware Overview

| Firmware line | Version | Status | Best use case | Installer |
| --- | --- | --- | --- | --- |
| BoxRFID OpenSpool Edition | V4.0 | Current release | Recommended for most users who want QIDI + OpenSpool in one firmware | Yes |
| BoxRFID OpenSpool Edition | V3.7 | Stable fallback | Previous combined release kept as fallback if you want to step back from V4.0 | Yes |
| BoxRFID-Touch | V2.1 | Stable classic | Classic QIDI-only workflow | Yes |
| BoxRFID-Touch | V2.0 | Legacy | Older classic QIDI release kept only for compatibility | No |

Public firmware folders in this repository:

- [BoxRFID-Touch V2.0](./firmware/boxrfid-touch/v2.0/)
- [BoxRFID-Touch V2.1](./firmware/boxrfid-touch/v2.1/)
- [BoxRFID OpenSpool Edition V3.7](./firmware/boxrfid-openspool/v3.7/)
- [BoxRFID OpenSpool Edition V4.0](./firmware/boxrfid-openspool/v4.0/)

Source folders:

- [BoxRFID-Touch V2.1 source](./source/boxrfid-touch/v2.1/)
- [BoxRFID OpenSpool Edition V3.7 source](./source/boxrfid-openspool/v3.7/)
- [BoxRFID OpenSpool Edition V4.0 source](./source/boxrfid-openspool/v4.0/)

Documentation:

- [Firmware Matrix](./docs/firmware-matrix.md)
- [BoxRFID-Touch V2.1 documentation](./docs/BoxRFID-Touch/BoxRFID-Touch.md)
- [BoxRFID OpenSpool Edition V4.0 documentation](./docs/BoxRFID-OpenSpool-Edition/BoxRFID-OpenSpool-Edition.md)

---

## What It Does

BoxRFID-Touch lets you read and write compatible filament RFID/NFC tags directly on the device without needing a PC during normal use.

Core capabilities across the firmware lines:

- manual tag reading
- automatic tag reading on the main screen
- direct tag writing on the touchscreen
- multilingual user interface
- persistent setup storage
- local editable material and manufacturer lists

Additional capabilities in the OpenSpool Edition:

- QIDI mode and OpenSpool mode in one firmware
- OpenSpool Standard and OpenSpool U1 / Extended workflows
- flexible color, HEX, numeric, and variant inputs
- per-model QIDI databases
- MicroSD and Wi-Fi based official QIDI CFG support in V4.0

---

## What's New In V4.0

Major changes since the public V3.7 release:

- added **QIDI Max 4** support alongside **QIDI Q2** and **QIDI Plus 4**
- added **Wi-Fi setup** with persistent SSID and password storage
- added a simple **web browser upload page** for `officiall_filas_list.cfg`
- added **MicroSD-based official QIDI CFG support** for all three QIDI printer models
- added **per-model enable toggles** for the official QIDI CFG files
- redesigned the **QIDI CFG setup page** with clear model status buttons and toggles
- added **on-device SD card formatting** that recreates the required QIDI folder structure
- improved the **password keyboard** and reused the extended keyboard for general text entry
- improved **touch, SD, and Wi-Fi interaction stability**
- improved **status text** so the active QIDI model is visible during read and write workflows
- kept **V3.7** in the installer as a fallback path if users want to step back from V4.0

---

## Which Version Should I Use

- Choose **BoxRFID OpenSpool Edition V4.0** if you want the latest release with QIDI + OpenSpool support and all current improvements.
- Choose **BoxRFID OpenSpool Edition V3.7** if you want the previous combined release as a fallback.
- Choose **BoxRFID-Touch V2.1** if you only need the classic QIDI firmware line.
- Use **BoxRFID-Touch V2.0** only if you specifically need the older classic firmware.

---

## Installation

### Web Installer

1. Connect the ESP32 board via USB.
2. Open the [Web Installer](https://tinkerbarn.github.io/BoxRFID-Touch/).
3. Click **Connect**.
4. Select the correct serial port.
5. Choose the desired firmware:
   - `V4.0` for the latest combined release
   - `V3.7` for the combined fallback release
   - `V2.1` for the classic QIDI release
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

### BoxRFID OpenSpool Edition V4.0

QIDI support:

- **QIDI Q2**
- **QIDI Plus 4**
- **QIDI Max 4**

OpenSpool support:

- **OpenSpool standard tags**
- **Snapmaker U1** with **paxx12 extended firmware**
- **Snapmaker U1** with **OpenRFID support**

### BoxRFID-Touch V2.1

Classic firmware line for:

- **classic QIDI-style workflow**
- **MIFARE Classic 1K tags**

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

> Note: The UI screenshots below currently show an older interface and not the current BoxRFID OpenSpool Edition V4.0 release.

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
