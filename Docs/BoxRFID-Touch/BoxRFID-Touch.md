# BoxRFID-Touch V2.1 – Feature Overview

This document describes the actual functionality implemented in the stable **BoxRFID-Touch V2.1** sketch for the **ESP32-2432S028R (CYD)** with **PN532 via I2C**.

The content below is based directly on the firmware source code and is intended for publication as a GitHub documentation page.

---

## Overview

**BoxRFID-Touch V2.1** is a standalone touchscreen RFID reader/writer with an integrated BLE bridge for QIDI-compatible RFID tags.

It combines:

- a graphical touchscreen user interface on the CYD display
- direct RFID reading and writing with a PN532 module
- local configuration and database management on the device
- Bluetooth Low Energy communication for external apps or tools

The firmware is designed for **MIFARE Classic 1K style workflows** and stores tag data in a compact format using:

- **Material ID**
- **Color ID**
- **Manufacturer ID**

These values are written to and read from **data block 4**.

---

## Supported Hardware

The sketch is written for the following hardware configuration:

- **ESP32-2432S028R** (CYD / Cheap Yellow Display style 2.8" resistive touch board)
- **ILI9341** TFT display via `TFT_eSPI`
- **XPT2046** resistive touch controller
- **PN532** connected via **I2C**

Configured PN532 pins in the sketch:

- **SDA:** IO27
- **SCL:** IO22

Configured touch controller pins in the sketch:

- **IRQ:** IO36
- **MOSI:** IO32
- **MISO:** IO39
- **CLK:** IO25
- **CS:** IO33

---

## Main Functions

### 1. Read Tag

The firmware can read supported RFID tags directly from the touchscreen UI.

During a read operation, the firmware:

1. waits for a tag
2. authenticates the selected data block using the default MIFARE Classic Key A
3. reads the stored data from **block 4**
4. decodes the values as:
   - material
   - color
   - manufacturer
5. shows the decoded information in a popup on the display

If reading fails, clear on-screen status messages are shown for situations such as:

- no tag detected
- NFC busy
- authentication failure
- read failure

---

### 2. Write Tag

The firmware can write tag data directly from the touchscreen UI.

Before writing, the user can choose:

- **Manufacturer**
- **Material**
- **Color**

The firmware then writes these values into **block 4** of the RFID tag.

Current data layout used by the sketch:

- `data[0]` = material ID
- `data[1]` = color ID
- `data[2]` = manufacturer ID

The remaining bytes of the 16-byte block are left unused by the normal UI write function.

As with reading, the firmware handles and displays write errors such as:

- no tag detected
- NFC busy
- authentication failure
- write failure

---

### 3. Auto Read on Main Screen

A built-in **auto-detect / auto-read mode** is available directly from the main screen.

When enabled:

- the firmware continuously checks for a supported RFID tag while the main screen is open
- detected tag information is shown automatically in a popup panel
- the popup updates when the tag contents change
- the popup disappears again after the tag is removed

This makes it possible to quickly inspect tags without manually opening the dedicated read screen every time.

The auto-read feature can be toggled directly on the main screen with an **Auto: ON / Auto: OFF** button.

---

## Touchscreen User Interface

The firmware includes a complete local touchscreen UI with dedicated screens for all major tasks.

Implemented UI sections in the sketch:

- **Main menu**
- **Read screen**
- **Write screen**
- **Material selection**
- **Color selection**
- **Manufacturer selection**
- **Setup screen**
- **Language selection**
- **Material management screens**
- **Manufacturer management screens**
- **Confirmation dialogs**
- **Message / notice dialogs**
- **On-screen keyboard**

The interface uses a header area, a status bar, and large touch-friendly buttons optimized for the CYD display.

---

## Material Management

The firmware contains an internal editable material database stored in ESP32 preferences.

### Default materials

The sketch includes a predefined list of materials such as:

- PLA
- PETG
- ABS
- ASA
- TPU
- PLA Silk
- PLA Matte
- PLA Wood
- PA12-CF
- PAHT-CF
- PPS-CF
- PVA
- and more

The default database in this version contains **more than 30 predefined materials**.

### Material database functions

From the setup menu, the user can:

- open the material menu
- edit existing material names
- assign names to unused material slots
- restore the material list to factory defaults

The sketch supports up to **50 material slots**.

This allows the firmware to be adapted to different filament collections or naming preferences without recompiling the code.

---

## Manufacturer Management

The firmware also contains an editable manufacturer database stored in ESP32 preferences.

### Default manufacturers

The default sketch database includes:

- **Generic**
- **QIDI**

### Manufacturer database functions

From the setup menu, the user can:

- open the manufacturer menu
- edit existing manufacturer names
- assign names to unused manufacturer slots
- restore the manufacturer list to factory defaults

The sketch supports up to **24 manufacturer slots**.

This allows the tag system to be adapted to different brands or workflows.

---

## Color Selection

Color handling in V2.1 is based on a predefined internal color table.

The firmware contains **24 selectable colors**, each with:

- a numeric color ID
- a display color in RGB565
- a translated label for the UI

Examples include:

- White
- Black
- Gray
- Blue
- Red
- Yellow
- Green
- Orange
- Silver
- Brown
- Violet
- Bronze

### Color UI behavior

The write screen shows the currently selected color as a colored button.
The color picker screen displays the available colors as a grid of selectable color boxes.
The firmware also automatically chooses a readable text color depending on the selected background color.

Unlike the OpenSpool Edition, this V2.1 firmware does **not** use a free RGB color picker or direct HEX color input. It uses a fixed internal color list only.

---

## Multilingual User Interface

The sketch includes built-in multilingual UI support.

Implemented languages:

- German
- English
- Spanish
- Portuguese
- French
- Italian

The selected language is stored persistently in preferences and is restored after reboot.

This affects:

- main UI labels
- setup labels
- status messages
- color names
- dialogs and notices

---

## Display Inversion

V2.1 includes a dedicated **Display Inversion** option in the setup menu.

This feature allows the user to switch display inversion:

- **ON**
- **OFF**

The setting is stored persistently and automatically restored on startup.

This is useful because different CYD hardware variants or display modules may benefit from different display inversion settings.

---

## Touch Calibration

The firmware includes a built-in touch calibration routine.

Features:

- calibration can be started from the setup menu
- calibrated values are stored in preferences
- saved calibration is automatically loaded at boot
- invalid calibration values are rejected and replaced with defaults
- calibration can be cleared again through factory reset

This allows the firmware to compensate for unit-to-unit differences in resistive touch behavior.

---

## Factory Reset

A full **factory reset** function is available from the setup menu.

Factory reset clears or restores:

- touch calibration
- selected UI language
- display inversion setting
- material database
- manufacturer database

After reset, the firmware returns to the built-in default configuration.

---

## On-Screen Keyboard

V2.1 includes an integrated touchscreen keyboard for editing material and manufacturer names.

Implemented keyboard modes:

- **Uppercase**
- **Lowercase**
- **Numeric / symbol mode**

The keyboard supports convenient entry of:

- letters
- digits
- space
- dash `-`
- underscore `_`
- backspace
- clear field

This makes it possible to manage the internal databases directly on the device without external tools.

---

## BLE Bridge

A major feature of BoxRFID-Touch V2.1 is its built-in **Bluetooth Low Energy bridge**.

### BLE device name

The sketch advertises as:

- **BoxRFID-ESP32**

### BLE service model

The firmware uses a UART-style BLE service with:

- one RX characteristic for incoming commands
- one TX characteristic for notifications / responses

### BLE use case

This allows external clients such as mobile apps or custom tools to communicate with the RFID reader without using the local touchscreen UI.

---

## BLE Command Set

The sketch implements a simple text-based command protocol over BLE.

Supported commands found in the source code:

- `PING`
- `HELP`
- `PRESENT`
- `UID`
- `READ <block>`
- `WRITE <block> <32HEX>`

### Command behavior

- `PING` returns a simple ready response
- `HELP` returns the supported command list
- `PRESENT` checks whether a tag is currently present
- `UID` waits for a tag and returns its UID
- `READ <block>` reads a full 16-byte MIFARE block
- `WRITE <block> <32HEX>` writes a full 16-byte block from hex input

### BLE response examples implemented by the sketch

The source code contains responses such as:

- `OK READY`
- `OK CONNECTED`
- `OK PONG`
- `OK NO_TAG`
- `OK PLACE_TAG`
- `OK UID ...`
- `OK READ ...`
- `OK WRITE ...`
- `ERR BUSY`
- `ERR NO_TAG`
- `ERR AUTH_FAIL`
- `ERR READ_FAIL`
- `ERR WRITE_FAIL`
- `ERR UNKNOWN_CMD`

This makes the firmware usable both as a standalone device and as a BLE-controlled RFID utility.

---

## RFID Access Model

The firmware is designed around **MIFARE Classic authentication** using the default **Key A**.

In the standard UI workflow it uses:

- **block 4** for the application payload

The firmware does not implement a complex multi-block schema in this version. The UI-oriented tag format is intentionally compact and stores only the essential values required by the BoxRFID workflow.

---

## Persistent Storage

The sketch stores configuration and user data in ESP32 preferences.

Persistent data includes:

- touch calibration values
- selected language
- display inversion state
- material database entries
- manufacturer database entries

This ensures that the device keeps its configuration across reboots and power cycles.

---

## Status and Feedback System

The UI includes a dedicated status bar and message system.

The sketch provides visual feedback for states such as:

- BLE ready
- BLE connected
- ready to read
- waiting for tag
- tag detected
- write successful
- authentication failed
- NFC busy
- PN532 not found
- factory reset completed

This helps make the device understandable and usable without requiring a serial monitor.

---

## Intended Use Case

Based on the actual implementation in the sketch, **BoxRFID-Touch V2.1** is best described as:

- a standalone **QIDI-style RFID reader/writer**
- a compact **touchscreen tag programming device**
- a **BLE-enabled RFID bridge** for external software
- a configurable local tool for managing materials and manufacturers directly on the device

It is especially suited for users who want a simple and portable way to:

- read existing tags
- create or update tags
- inspect tag contents quickly using auto-read
- customize local material and manufacturer mappings
- access RFID functions through BLE commands

---

## Summary

**BoxRFID-Touch V2.1** provides the following core functionality:

- standalone RFID read and write functions
- compact tag format using material, color, and manufacturer IDs
- auto-read directly on the main screen
- editable material database
- editable manufacturer database
- fixed internal color database with 24 colors
- multilingual UI with 6 languages
- touch calibration stored in preferences
- display inversion option
- factory reset function
- integrated on-screen keyboard
- BLE UART-style command interface
- persistent storage of user settings and database changes

This makes V2.1 a stable and practical base firmware for BoxRFID-Touch in its classic QIDI-oriented form.
