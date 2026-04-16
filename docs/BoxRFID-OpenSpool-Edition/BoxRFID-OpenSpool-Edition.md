# BoxRFID OpenSpool Edition – Feature Overview

## Version
**BoxRFID OpenSpool Edition V3.6**

This document describes the feature set of the firmware version based on the actual Arduino sketch `BoxRFID_CYD_ESP32_2432S028R_Qidi_OpenSpool_V3.6.ino`.

BoxRFID OpenSpool Edition is a touchscreen-based standalone RFID reader and writer for the **ESP32-2432S028R (CYD)** with **PN532 I2C**, designed to work with both **QIDI RFID tags** and **OpenSpool tags**.

---

## Main purpose

The firmware allows the device to:

- read RFID spool tags directly on the touchscreen
- write new RFID spool tags without a computer or mobile app
- switch between **QIDI mode** and **OpenSpool mode**
- switch between **QIDI Plus 4** and **QIDI Q2** directly in the device setup
- manage separate material and manufacturer databases for both tag systems
- use an optimized OpenSpool read and write workflow with reduced tag access time

---

## Supported tag systems

### QIDI mode
In QIDI mode, the firmware supports the existing QIDI-style RFID workflow.

Functions include:

- reading QIDI tags
- writing QIDI tags
- selecting manufacturer, material, and color from internal lists
- switching between **QIDI Plus 4** and **QIDI Q2**
- using separate default material sets for both supported QIDI printer models

### OpenSpool mode
In OpenSpool mode, the firmware supports **OpenSpool JSON-based tags**.

Functions include:

- reading OpenSpool tags from NTAG-compatible media
- writing OpenSpool tags with `protocol: openspool` and `version: 1.0`
- storing the main OpenSpool fields directly on the tag
- showing the decoded tag content on screen after reading

The sketch supports two OpenSpool tag profiles:

#### 1. Standard OpenSpool tags
Standard OpenSpool tags can be created with:

- **brand / manufacturer**
- **material / type**
- **color_hex**
- **nozzle minimum temperature**
- **nozzle maximum temperature**
- **slicer page** with generated filament profile name

#### 2. Snapmaker U1 specific OpenSpool tags
The firmware also includes a dedicated **OpenSpool Snapmaker U1** profile for use with **Snapmaker U1** printers running **paxx12 extended firmware** with **OpenRFID support enabled**.

In the current V3.6 sketch, the U1 profile supports writing and reading the following U1-relevant OpenSpool fields:

- manufacturer / brand
- material / type
- color
- nozzle minimum temperature
- nozzle maximum temperature
- bed minimum temperature
- bed maximum temperature
- subtype / variant
- opacity value
- weight
- diameter
- up to four additional colors
- slicer page with generated filament profile name

The U1-specific fields are no longer controlled by a compile-time switch. Instead, they can be enabled or disabled individually in the on-device **Tag Information** settings for **OpenSpool Extended**.

---

## Main menu functions

The touchscreen main menu provides direct access to the core actions:

- **Read Tag**
- **Write Tag**
- **Settings**
- **Auto Read on/off**
- **QIDI / OpenSpool mode selection**

The interface is optimized for a standalone workflow, so common actions are reachable directly from the main screen.

---

## Read functions

### Manual tag reading
The firmware can read tags manually and show the detected content on screen.

Depending on the active mode, the screen displays the available tag information such as:

- manufacturer / brand
- material / type
- color
- OpenSpool subtype / variant
- OpenSpool temperature values
- additional OpenSpool detail pages where applicable
- slicer profile information as a dedicated final page

In OpenSpool mode, the tag information popup now uses a dynamic multi-page layout. Depending on the content present on the tag, the device can show:

- main page with manufacturer, material / variant, and color
- temperature page with nozzle, bed, and opacity
- detail page with weight and diameter
- additional colors page
- slicer page with the generated filament profile name

### Auto Read
The firmware also includes **Auto Read**.

When enabled, the device continuously checks for nearby tags and automatically shows the tag information when a supported tag is detected.

This is useful for:

- quickly checking spool tags without pressing buttons
- comparing multiple tags one after another
- fast verification of newly written tags

---

## Write functions

### QIDI tag writing
In QIDI mode, the firmware can write supported QIDI tag data using the selected material, color, and manufacturer values.

### OpenSpool tag writing
In OpenSpool mode, the firmware can create and write OpenSpool JSON tags.

The write workflow supports:

- selecting the OpenSpool profile type (**Standard OpenSpool** or **OpenSpool Snapmaker U1**)
- selecting manufacturer
- selecting material
- selecting color
- entering optional Standard or U1-specific values depending on the active profile and enabled fields
- a dedicated final **Slicer** page with the generated filament profile name

The firmware automatically chooses the best data payload size that fits on the tag.

---

## Color handling in OpenSpool mode

A major feature of the OpenSpool workflow is the flexible color input system.

In OpenSpool mode, colors can be set in three different ways:

1. **Predefined color list**
2. **Integrated color picker**
3. **Direct HEX RGB input**

The color picker includes:

- hue/saturation/value based color selection
- live preview of the selected color
- conversion to normalized HEX color format

Direct HEX entry is useful when an exact spool color value is known and should be stored precisely on the tag.

For OpenSpool Extended opacity input, the firmware supports:

- a percentage-based slider
- direct HEX entry
- exact preservation of manually entered HEX opacity values

---

## Optimized keyboard layouts

The sketch includes multiple on-screen keyboard modes that are optimized for the current input type.

Available keyboard layouts include:

- **text keyboard** for names and labels
- **numeric keyboard** for number-only entries
- **numeric keyboard with decimal support** for values such as filament diameter
- **HEX keyboard** for hexadecimal input such as RGB/alpha values

This makes input easier and more reliable, especially on the 2.8-inch touchscreen.

---

## Material management

The firmware contains separate editable material databases for:

- **QIDI mode**
- **OpenSpool mode**

Functions include:

- selecting materials for tag writing
- editing existing materials
- adding new materials
- restoring the material list to factory defaults

For OpenSpool materials, the firmware also supports storing nozzle temperature presets.
For OpenSpool materials, the firmware also supports storing bed temperature presets.

The sketch already contains built-in presets for common materials such as:

- PLA
- PETG
- ABS
- ASA
- TPU
- PA
- PA12
- PC
- PEEK
- PVA
- HIPS
- PCTG
- PLA-CF
- PETG-CF
- PA-CF

---

## Manufacturer management

The firmware contains separate editable manufacturer databases for:

- **QIDI mode**
- **OpenSpool mode**

Functions include:

- selecting manufacturers for tag writing
- editing manufacturer entries
- adding new manufacturer entries
- restoring manufacturer lists to factory defaults

In QIDI mode, manufacturer and material handling follow the currently selected printer model (**Plus 4** or **Q2**).

The OpenSpool default list already contains entries such as **Generic**, **QIDI**, **Snapmaker**, **Bambu Lab**, **Prusament**, **eSUN**, **Polymaker**, and others defined in the sketch.

---

## Multilingual user interface

The firmware includes a multilingual touchscreen UI.

Languages present in the sketch are:

- German
- English
- Spanish
- Portuguese
- French
- Italian

Language selection is available directly from the settings menu.

---

## Settings and UI features

The settings menu includes multiple configurable system options.

### Default startup mode
The firmware can store the preferred default tag mode:

- QIDI
- OpenSpool

### OpenSpool Tag Information settings
The OpenSpool settings include dedicated configuration pages for **Standard** and **Extended** tag information handling.

Available options include:

- enabling or disabling nozzle information in OpenSpool Standard
- enabling or disabling bed temperature in OpenSpool Extended
- enabling or disabling opacity in OpenSpool Extended
- enabling or disabling weight in OpenSpool Extended
- enabling or disabling diameter in OpenSpool Extended
- enabling or disabling additional colors in OpenSpool Extended
- selecting the automatic page switch interval for OpenSpool read pages

### Display brightness
Brightness is adjustable and stored persistently.

The sketch uses percentage-based brightness control and remembers the selected level across reboots.

### Configurable screensaver
The firmware includes a **configurable screensaver**.

Available timeout options in the sketch are:

- 30 seconds
- 1 minute
- 5 minutes
- 10 minutes
- off

When the screensaver becomes active, the display brightness is reduced automatically. When activity resumes, the previous brightness is restored.

The screensaver is also designed to react to:

- touch activity
- RFID tag presence

### Display inversion
The settings menu includes a **display inversion** toggle.

### Touch calibration
The firmware supports touchscreen calibration and stores the calibration values in preferences.

### Factory reset
A factory reset function restores the saved UI settings and internal lists to their default state.

---

## Persistent storage

The firmware stores user settings in ESP32 preferences, including:

- language
- display inversion
- auto read state
- default tag mode
- selected QIDI printer model
- screensaver mode
- brightness
- OpenSpool tag information options
- touch calibration
- material lists
- manufacturer lists

This ensures the device keeps its configuration after reboot or power loss.

---

## Installation

This firmware can be provided through the **BoxRFID-Touch Web Installer**, allowing installation without manually compiling the sketch in the Arduino IDE.

For repository documentation, this is useful because it gives end users two possible installation paths:

- flashing through the **Web Installer**
- building and uploading the sketch manually in the Arduino IDE

---

## Hardware platform

The sketch is written for:

- **ESP32-2432S028R (CYD / Cheap Yellow Display)**
- **2.8-inch ILI9341 TFT display**
- **XPT2046 resistive touch controller**
- **PN532 connected via I2C**

---

## Summary

**BoxRFID OpenSpool Edition V3.6** is a standalone touchscreen RFID tool for reading and writing both **QIDI** and **OpenSpool** spool tags.

Its key strengths are:

- direct on-device read and write workflow
- support for both **QIDI Plus 4** and **QIDI Q2**
- support for both **standard OpenSpool tags** and **Snapmaker U1 specific OpenSpool tags**
- flexible OpenSpool color handling via predefined colors, color picker, and HEX input
- configurable OpenSpool Standard and Extended tag information fields
- dynamic multi-page OpenSpool tag information display
- dedicated slicer profile page for OpenSpool tags
- optimized keyboards for text, numbers, and hexadecimal values
- editable material and manufacturer databases
- multilingual interface
- configurable screensaver and brightness system
- installation via the BoxRFID-Touch Web Installer

---
