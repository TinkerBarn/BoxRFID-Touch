# BoxRFID OpenSpool Edition V4.0

This document describes the published **BoxRFID OpenSpool Edition V4.0** firmware for the **ESP32-2432S028R (CYD)** with **PN532 via I2C**.

V4.0 is the current all-in-one BoxRFID release. It combines QIDI Box support and OpenSpool support in one firmware and adds the major platform updates introduced after V3.7.

---

## What is new in V4.0

Compared with the public V3.7 release, V4.0 adds and refines:

- support for **QIDI Max 4** in addition to **QIDI Q2** and **QIDI Plus 4**
- persistent **Wi-Fi setup** with SSID and password storage
- a built-in **browser uploader** for `officiall_filas_list.cfg`
- `MicroSD` support for storing official QIDI configuration files
- per-model QIDI CFG handling for `Q2`, `Plus 4`, and `Max 4`
- setup toggles to enable or disable the official QIDI CFG for each supported printer model
- a redesigned QIDI CFG setup page with clear status colors
- manual MicroSD workflow support using:
  - `/qidi/q2/officiall_filas_list.cfg`
  - `/qidi/plus4/officiall_filas_list.cfg`
  - `/qidi/max4/officiall_filas_list.cfg`
- an on-device **SD card format** function that rebuilds the folder structure
- improved text keyboard and password keyboard layouts
- the extended keyboard now reused for general name entry
- better touch and SPI handling for the combined touch, Wi-Fi, and SD workflow
- QIDI model information shown in relevant read and write status text

---

## Hardware and tag support

Supported standalone workflows:

- `OpenSpool Standard` RFID/NFC tags
- `Snapmaker U1` with `paxx12 Extended Firmware`
- `QIDI Q2`, `QIDI Plus 4`, and `QIDI Max 4` with `QIDI Box`

Target hardware:

- `ESP32-2432S028R (CYD)`
- `ILI9341` display
- `XPT2046` resistive touch
- `PN532` in `I2C` mode
- optional `MicroSD` card in the built-in CYD slot

Supported RFID tag families:

- `MIFARE Classic 1K` for QIDI mode
- `NTAG215 / NTAG-compatible` OpenSpool tags for OpenSpool mode

---

## Firmware modes

V4.0 contains two main operating modes.

### QIDI mode

QIDI mode supports:

- reading QIDI-style tags
- writing QIDI-style tags
- working with `QIDI Box` on supported QIDI printers
- selecting the active printer model:
  - `QIDI Q2`
  - `QIDI Plus 4`
  - `QIDI Max 4`
- separate material and manufacturer databases for each supported QIDI printer
- optional use of the official `officiall_filas_list.cfg` file for each printer model

### OpenSpool mode

OpenSpool mode supports:

- reading OpenSpool JSON/NDEF tags
- writing OpenSpool tags
- `OpenSpool Standard` workflow
- extended `Snapmaker U1 / paxx12 Extended Firmware` workflow
- `Snapmaker U1 / OpenRFID` workflow

---

## Main menu

The main screen gives direct access to:

- `Auto Read` toggle
- `Read Tag`
- `Setup`
- `Write Tag`
- `Mode switch` between `QIDI` and `OpenSpool`

The status bar at the bottom shows version information or live status information depending on the current action.

---

## Read functions

### QIDI read

In QIDI mode the firmware:

1. waits for a compatible tag
2. authenticates MIFARE block 4
3. reads material, color, and manufacturer IDs
4. shows the decoded values in a popup

The popup title includes the active printer model, for example:

- `Tag Information QIDI Q2`
- `Tag Information QIDI Plus 4`
- `Tag Information QIDI Max 4`

### OpenSpool read

In OpenSpool mode the firmware reads the OpenSpool payload and shows the result in one or more pages, depending on the stored fields.

Possible OpenSpool read pages include:

- base page with brand, type, subtype, and color
- temperature page
- alpha / opacity page
- weight / diameter page
- additional colors page
- slicer profile page

### Auto read

Auto read is available from the main screen in both modes.

When enabled, the device automatically scans for nearby supported tags and opens the information popup when a tag is detected.

---

## Write functions

### QIDI write

In QIDI mode, the write screen lets the user select:

- manufacturer
- material
- color

The firmware then writes the selected values to block 4.

The status line includes the active QIDI model while writing, which makes it clear which printer-specific database is currently in use.

### OpenSpool write

In OpenSpool mode, the write flow is multi-page and depends on the selected profile.

#### Standard OpenSpool

Standard OpenSpool pages can include:

- brand
- material / type
- color
- nozzle min / max
- slicer profile preview

#### OpenSpool U1 / Extended

Extended OpenSpool pages can include:

- brand
- material / type
- color
- variant / subtype
- nozzle min / max
- optional bed min / max
- optional opacity
- optional weight
- optional diameter
- optional additional colors 1 to 4
- slicer profile preview

The firmware automatically selects the best payload tier that fits on the tag and warns the user if only a reduced payload could be stored.

---

## Setup menu pages

V4.0 uses a five-page setup structure.

### Page 1/5: Core setup

Depending on mode and whether an official QIDI CFG is active, page 1 contains:

- manufacturer editor
- material editor
- language selection
- default startup mode
- active QIDI printer model selector

If an official QIDI CFG is active for the current model, manual QIDI material/manufacturer editing is intentionally hidden for that model.

### Page 2/5: Screensaver and brightness

Page 2 contains:

- screensaver timeout
- display brightness

Screensaver choices:

- `30 seconds`
- `1 minute`
- `5 minutes`
- `10 minutes`
- `Off`

### Page 3/5

The content of page 3 depends on the active tag mode.

#### QIDI mode page 3

If no MicroSD card is available, the page shows a clear warning.

If a MicroSD card is available, the page shows one status row per supported QIDI model:

- `Q2`
- `Plus 4`
- `Max 4`

Each row contains:

- a large status button
- an enable/disable toggle

Status logic:

- red = CFG file missing
- green = CFG file present
- the right-side toggle determines whether the file is actively used

The page also contains:

- `Format SD card`

Tapping a model row opens an info dialog explaining:

- that the user should obtain `officiall_filas_list.cfg` from Klipper
- that it can be copied manually to MicroSD
- or uploaded from a web browser when Wi-Fi is connected

#### OpenSpool mode page 3

Page 3 opens the OpenSpool tag information configuration pages:

- `OpenSpool Standard`
- `OpenSpool Extended`

### Page 4/5: Wi-Fi

The Wi-Fi page contains:

- Wi-Fi enable/disable
- SSID field
- password field
- live IP address / network state button

Wi-Fi data stored persistently:

- Wi-Fi enabled flag
- SSID
- password

The IP address is shown live and is not stored persistently.

### Page 5/5: Device settings

The final page contains:

- display inversion
- touch calibration
- factory defaults

---

## QIDI official CFG support

V4.0 adds per-model support for the official QIDI material list file:

- `Q2` uses `/qidi/q2/officiall_filas_list.cfg`
- `Plus 4` uses `/qidi/plus4/officiall_filas_list.cfg`
- `Max 4` uses `/qidi/max4/officiall_filas_list.cfg`

### When the official CFG is enabled

If a CFG file exists and its toggle is enabled for the selected model:

- QIDI materials are loaded from the official file
- QIDI manufacturers are loaded from the official file
- manual edit/add menus for QIDI materials and manufacturers are hidden for that model

### When the official CFG is disabled

If the toggle is off:

- the firmware falls back to the internal persistent/default QIDI lists
- manual QIDI editing becomes available again

### Browser upload

When Wi-Fi is connected, the device serves a small upload page on the live IP address.

The upload page allows direct upload of:

- `Q2` CFG
- `Plus 4` CFG
- `Max 4` CFG

### Manual MicroSD workflow

Users can also update the CFGs without Wi-Fi by copying the files manually to the correct MicroSD folders.

---

## Material, manufacturer, and variant editors

### QIDI databases

QIDI databases are separated by printer model. Each model can have its own:

- material list
- manufacturer list

### OpenSpool databases

OpenSpool provides editable persistent lists for:

- manufacturers
- materials
- variants / subtypes

For OpenSpool materials, the editor supports:

- name
- nozzle min
- nozzle max
- bed min
- bed max

---

## Keyboard and input modes

V4.0 includes multiple input keyboards chosen automatically by field type.

### Extended text keyboard

Used for:

- SSID
- Wi-Fi password
- material names
- manufacturer names
- variant names
- other general text entry

It includes:

- uppercase page
- lowercase page
- numeric page
- two symbol pages

### Numeric keyboard

Used for:

- temperatures
- weight
- other numeric fields

### Numeric with decimal support

Used for:

- diameter fields

### HEX keyboard

Used for:

- color hex
- opacity hex

---

## Persistent storage

V4.0 stores persistent settings in ESP32 preferences, including:

- UI language
- default tag mode
- last used mode
- selected QIDI printer model
- brightness
- screensaver
- display inversion
- touch calibration
- Wi-Fi enabled state
- Wi-Fi SSID
- Wi-Fi password
- QIDI official CFG toggles
- QIDI lists
- OpenSpool lists
- OpenSpool draft values
- OpenSpool tag info options

---

## Typical procedures

### Write a QIDI tag

1. Switch to `QIDI` mode.
2. Select the correct printer model in `Setup`.
3. Open `Write Tag`.
4. Select manufacturer, material, and color.
5. Tap `Write Tag`.
6. Place the MIFARE Classic tag on the reader.

### Read a QIDI tag

1. Switch to `QIDI` mode.
2. Tap `Read Tag`, or enable `Auto Read`.
3. Place the tag on the reader.
4. Check the popup title to confirm the active QIDI model.

### Upload an official QIDI CFG over Wi-Fi

1. Insert a MicroSD card.
2. Open `Setup > Wi-Fi`.
3. Enable Wi-Fi and enter SSID and password.
4. Wait for the device to show an IP address.
5. Open the shown URL in a browser.
6. Upload the correct `officiall_filas_list.cfg` for `Q2`, `Plus 4`, or `Max 4`.
7. Return to `Setup > QIDI CFG`.
8. Enable the toggle for the model you want to use.

### Copy an official QIDI CFG manually

1. Power off the device or remove the MicroSD card safely.
2. Copy the file to one of these paths:
   - `/qidi/q2/officiall_filas_list.cfg`
   - `/qidi/plus4/officiall_filas_list.cfg`
   - `/qidi/max4/officiall_filas_list.cfg`
3. Reinsert the MicroSD card.
4. Open the `QIDI CFG` setup page.
5. Enable the toggle for the matching model.

### Format the SD card structure from the device

1. Insert a MicroSD card.
2. Open `Setup > QIDI CFG`.
3. Tap `Format SD card`.
4. Confirm the action.
5. Let the device rebuild the required QIDI folder structure.

### Write a Standard OpenSpool tag

1. Switch to `OpenSpool` mode.
2. Open `Write Tag`.
3. Choose `OpenSpool Standard`.
4. Set brand, type, color, and optional nozzle temperatures.
5. Review the slicer profile preview.
6. Tap `Write`.
7. Place the NTAG tag on the reader.

### Write an OpenSpool U1 / Extended tag

1. Switch to `OpenSpool` mode.
2. Open `Write Tag`.
3. Choose `OpenSpool U1`.
4. Fill in the enabled pages and optional fields.
5. Review the slicer profile preview.
6. Tap `Write`.
7. Place the NTAG tag on the reader.

---

## Summary

BoxRFID OpenSpool Edition V4.0 is the current release for users who want:

- QIDI and OpenSpool in one firmware
- support for QIDI `Q2`, `Plus 4`, and `Max 4`
- official QIDI CFG file support through MicroSD and browser upload
- persistent Wi-Fi configuration
- improved keyboard layouts and setup flow
- editable local OpenSpool databases
- a fallback path back to internal QIDI lists when official CFG use is disabled

It is the recommended release for new installations.
