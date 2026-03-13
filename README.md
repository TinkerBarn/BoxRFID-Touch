# BoxRFID-Touch – Standalone RFID Tag Tool for QIDI Box

<p align="center">
  <a href="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Home.jpeg">
    <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/Front.jpeg" height="320" alt="BoxRFID-Touch Home Screen">
  </a>
</p>

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

BoxRFID-Touch uses the PN532 RFID sensor in **I2C mode**. Don't forget to change the DIP switch on the PN532 to the I2C mode. See detailed screenshots below.
Here you see the Pinout for connecting the PN532 RFID board to the ESP32-2432S028R.

### PINOUT CYD ↔ PN532

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
```

### Overview of necessary parts 
<table align="center">
  <tr>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Electronic parts.jpeg" height="180" alt="Electronic parts"><br>
      <sub>Electronic parts</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/PN532.jpeg" height="180" alt="PN532 RFID Sensor"><br>
      <sub>PN532 RFID Sensor</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/PN532-I2C.jpeg" height="180" alt="Set to I2C"><br>
      <sub>Set PN532 to I2C</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/ESP32-2432S028R.jpeg" height="180" alt="ESP32-2432S028R"><br>
      <sub>ESP32-2432S028R</sub>
    </td>
  </tr>
</table>

### Details how to connect the boards

<table align="center">
  <tr>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Cable connection.jpeg" height="180" alt="Connect the cables"><br>
      <sub>Connect the cables</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Detail cable.jpeg" height="180" alt="Detail view ESP32"><br>
      <sub>Detail view ESP32</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Detail PN532.jpeg" height="180" alt="Detail view PN532"><br>
      <sub>Detail view PN532</sub>
    </td>
   </tr>
</table>

### Case mounting 
<table align="center">
  <tr>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Place PN532.jpeg" height="180" alt="Place PN532"><br>
      <sub>Place PN532</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Place parts.jpeg" height="180" alt="Place ESP32 in case"><br>
      <sub>Place ESP32 in case</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Mount CYD.jpeg" height="180" alt="Mount CYD with M3 6mm screws"><br>
      <sub>Mount CYD with M3 6mm screws</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Mounted.jpeg" height="180" alt="All parts mounted"><br>
      <sub>All parts mounted</sub>
    </td>
  </tr>
    <tr>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Place PN532.jpeg" height="180" alt="Base plate and adhesive pads"><br>
      <sub>Base plate and adhesive pads</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Base plate with feeds.jpeg" height="180" alt="Base plate and mounted pads"><br>
      <sub>Base plate and mounted pads</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Close Case.jpeg" height="180" alt="Close the case"><br>
      <sub>Close the case</sub>
    </td>
    <td align="center">
      <img src="https://github.com/TinkerBarn/BoxRFID-Touch/blob/main/screenshots/Screw01.jpeg" height="180" alt="M3 5mm countersunk screw"><br>
      <sub>M3 5mm countersunk screw</sub>
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

## SCREENSHOTS

<table align="center">
  <tr>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/HomeDetailed.jpeg" height="180" alt="Home Detailed"><br>
      <sub>Home</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/ReadTag.jpeg" height="180" alt="Read Tag"><br>
      <sub>Read Tag</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/ReadManually.jpeg" height="180" alt="Read Manually"><br>
      <sub>Read Manually</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/WriteMain.jpeg" height="180" alt="Write Main"><br>
      <sub>Write Main</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/WriteMaterial.jpeg" height="180" alt="Write Material"><br>
      <sub>Write Material</sub>
    </td>
  </tr>
  <tr>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/WriteColor.jpeg" height="180" alt="Write Color"><br>
      <sub>Write Color</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/SetupMain.jpeg" height="180" alt="Setup Main"><br>
      <sub>Setup Main</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/SetupLanguage.jpeg" height="180" alt="Setup Language"><br>
      <sub>Setup Language</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/SetupManufacturer.jpeg" height="180" alt="Setup Manufacturer"><br>
      <sub>Setup Manufacturer</sub>
    </td>
    <td align="center">
      <img src="https://raw.githubusercontent.com/TinkerBarn/BoxRFID-Touch/main/screenshots/SetupMaterial.jpeg" height="180" alt="Setup Material"><br>
      <sub>Setup Material</sub>
    </td>
  </tr>
</table>
