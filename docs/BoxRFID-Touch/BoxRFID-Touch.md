# BoxRFID-Touch V2.1

This document describes the published **BoxRFID-Touch V2.1** firmware for the **ESP32-2432S028R (CYD)** with **PN532 via I2C**.

V2.1 is the classic BoxRFID firmware line. It focuses on the original QIDI-style RFID workflow and does not include OpenSpool mode.

---

## Overview

BoxRFID-Touch V2.1 is a standalone touchscreen RFID tool that can:

- read compatible QIDI-style RFID tags
- write manufacturer, material, and color data to compatible tags
- auto-read tags on the main screen
- manage local material and manufacturer lists directly on the device
- expose a BLE command interface for external tools

The firmware stores compact QIDI-style payload data in **MIFARE Classic block 4**.

---

## Hardware

Target hardware:

- `ESP32-2432S028R (CYD)`
- `ILI9341` display
- `XPT2046` resistive touch
- `PN532` in `I2C` mode

Configured PN532 pins:

- `SDA = GPIO 27`
- `SCL = GPIO 22`

Configured touch pins:

- `IRQ = GPIO 36`
- `MOSI = GPIO 32`
- `MISO = GPIO 39`
- `CLK = GPIO 25`
- `CS = GPIO 33`

---

## Supported workflow

V2.1 is intended for the classic BoxRFID / QIDI tag workflow based on:

- `MIFARE Classic 1K` tags
- authentication with the default MIFARE key
- compact data stored in block 4

Payload mapping used by the UI:

- `data[0]` = material ID
- `data[1]` = color ID
- `data[2]` = manufacturer ID

---

## Main menu

The main screen provides the core operating flow:

- `Read Tag`
- `Write Tag`
- `Setup`
- `Auto Read` toggle

The status bar gives direct feedback about readiness, successful reads or writes, and common failure states.

---

## Read options

### Manual read

The manual read screen waits for a compatible tag, authenticates block 4, reads the payload, and shows:

- manufacturer
- material
- color

Common status/error feedback:

- no tag detected
- NFC busy
- authentication failed
- read failed

### Auto read

When `Auto Read` is enabled on the main screen, V2.1 continuously checks for tags while the home screen is open.

Detected tags are shown automatically in a popup panel. The panel updates when the tag changes and disappears after tag removal.

---

## Write options

The write screen allows the user to select:

- manufacturer
- material
- color

After tapping `Write Tag`, the firmware waits for a tag, authenticates block 4, and writes the selected IDs.

This is the standard workflow for programming a new tag or updating an existing tag to match a spool.

---

## Selection screens

V2.1 includes dedicated picker screens for:

- material selection
- manufacturer selection
- color selection

The color picker uses a fixed internal color table. It does not provide a free RGB picker or raw HEX input.

---

## Material management

The firmware contains a persistent editable material database.

Default material coverage includes many common filament types such as:

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

Supported management actions:

- edit an existing material
- assign a free slot
- restore factory defaults

The firmware supports up to `50` material slots.

---

## Manufacturer management

V2.1 also contains a persistent editable manufacturer database.

Default manufacturers include:

- `Generic`
- `QIDI`

Supported management actions:

- edit an existing manufacturer
- assign a free slot
- restore factory defaults

The firmware supports up to `24` manufacturer slots.

---

## Setup menu

The setup menu gives access to the local configuration pages and editors.

Available settings and tools:

- material menu
- manufacturer menu
- language selection
- display inversion
- touch calibration
- factory reset

### Language

Built-in UI languages:

- German
- English
- Spanish
- Portuguese
- French
- Italian

The selected language is stored persistently.

### Display inversion

Display inversion can be toggled on or off and is saved across reboots.

### Touch calibration

Touch calibration can be started from setup. Calibration values are saved in preferences and restored automatically at boot.

### Factory reset

Factory reset restores the default configuration for:

- language
- display inversion
- touch calibration
- material list
- manufacturer list

---

## Keyboard

V2.1 includes an on-screen keyboard for text editing.

Available keyboard modes:

- uppercase
- lowercase
- numeric / symbols

It is used for:

- material names
- manufacturer names

---

## BLE bridge

V2.1 includes a BLE UART-style interface for external clients.

Advertised device name:

- `BoxRFID-ESP32`

Supported command examples:

- `PING`
- `HELP`
- `PRESENT`
- `UID`
- `READ <block>`
- `WRITE <block> <32HEX>`

This allows the device to act as both a standalone touchscreen tool and a BLE-controlled RFID helper for external software.

---

## Persistent storage

The firmware stores user configuration in ESP32 preferences, including:

- touch calibration
- language
- display inversion
- material database
- manufacturer database

This keeps the device configuration stable across power cycles.

---

## Typical procedures

### Read a tag

1. Power the device and wait for the main screen.
2. Tap `Read Tag`.
3. Place a compatible tag on the reader.
4. Wait for the popup with manufacturer, material, and color.

### Write a tag

1. Open `Write Tag`.
2. Select manufacturer.
3. Select material.
4. Select color.
5. Tap `Write Tag`.
6. Place the tag on the reader until the status confirms success.

### Change a material or manufacturer

1. Open `Setup`.
2. Open the material or manufacturer menu.
3. Choose edit, add, or reset.
4. Use the on-screen keyboard to update the entry.
5. Save the change.

### Recalibrate touch

1. Open `Setup`.
2. Start `Touch Calibration`.
3. Follow the on-screen prompts.
4. Return to the main screen after calibration is stored.

---

## Summary

BoxRFID-Touch V2.1 is the stable classic BoxRFID release for users who want:

- QIDI-style RFID reading and writing
- compact local tag programming
- editable local materials and manufacturers
- auto-read on the main screen
- BLE access for external tools

It is the simplest firmware line in this repository and remains the recommended option for users who only need the classic QIDI workflow.
