# BoxRFID OpenSpool Edition V4.2

This document describes the published **BoxRFID OpenSpool Edition V4.2** firmware for the **ESP32-2432S028R (CYD)** with **PN532 via I2C**.

V4.2 is the current all-in-one BoxRFID release. It combines QIDI Box support, OpenSpool support, and the Snapmaker U1 send workflow in one firmware. It builds on the V4.1 platform release and adds direct ToolHead sending, live ToolHead status display, and safer persistent preference handling.

---

## What is new in V4.2

Compared with V4.1, V4.2 adds and refines:

- direct **Tag senden** workflow for Snapmaker U1 ToolHeads
- direct entry into the ToolHead send menu instead of an intermediate send page
- localized title text for the Snapmaker U1 send menu
- localized read-tag action inside the ToolHead send menu
- QIDI and OpenSpool tag auto-detection while reading a tag for sending
- 3 second tag information popup before returning to the ToolHead send menu
- Snapmaker U1 ToolHead status refresh through GET requests
- ToolHead display with filament type and color, or `Leer` / `Empty` when no filament data is present
- automatic black/white text contrast on ToolHead buttons based on the displayed filament color
- filament-sensor check before sending to a ToolHead
- overwrite confirmation when filament is already detected in the selected ToolHead
- direct write without confirmation when no filament is detected
- automatic ToolHead status refresh after a successful send
- persistent Snapmaker host and port setup
- safer loading of stored lists and network settings to avoid crashes after custom material/manufacturer edits
- OpenSpool U1 tag writing clears the subtype field when `None` / `Keine` is selected again, so no `subtype` is written
- web installer release selection list

Compared with the older V3.7 release line, V4.x also includes:

- support for **QIDI Max 4** in addition to **QIDI Q2** and **QIDI Plus 4**
- persistent **Wi-Fi setup** with SSID and password storage
- a built-in **browser uploader** for `officiall_filas_list.cfg`
- `MicroSD` support for storing official QIDI configuration files
- per-model QIDI CFG handling for `Q2`, `Plus 4`, and `Max 4`
- setup toggles to enable or disable the official QIDI CFG for each supported printer model
- a redesigned QIDI CFG setup page with clear status colors
- automatic backup of user setup and edited lists to MicroSD when a card is present
- automatic restore after firmware updates or when moving the MicroSD card to another BoxRFID device
- mDNS access for the web interface via `http://boxrfid.local`
- manual MicroSD workflow support using:
  - `/qidi/q2/officiall_filas_list.cfg`
  - `/qidi/plus4/officiall_filas_list.cfg`
  - `/qidi/max4/officiall_filas_list.cfg`
- an on-device **SD card format** function that rebuilds the folder structure
- an SD tools page with on-device content view
- improved web CFG page with file status, card-content listing, and direct CFG content viewing
- runtime MicroSD hotplug detection on the relevant setup pages
- MicroSD-based backup and restore for setup, material, manufacturer, and variant changes
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

V4.2 contains two main operating modes plus a Snapmaker U1 send workflow.

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
- `Tag senden`
- `Mode switch` between `QIDI` and `OpenSpool`

The status bar at the bottom shows version information or live status information depending on the current action.

## Snapmaker U1 send workflow

V4.2 adds a dedicated send workflow for transferring the currently read tag data to a Snapmaker U1 ToolHead.

### Requirements

The send workflow requires:

- Wi-Fi enabled on BoxRFID
- BoxRFID connected to the same network as the Snapmaker U1
- Snapmaker host and port configured in setup
- Snapmaker U1 running firmware/API support that accepts the filament GET and SET requests used by the workflow

If Wi-Fi is disabled or the U1 connection data is missing, the ToolHead menu shows the connection state and does not offer a tag-read/send path.

### ToolHead menu

Tapping the send action on the main screen opens the ToolHead menu directly.

The menu title follows the selected UI language.

The read action in this menu also follows the selected UI language.

The four ToolHead buttons show the latest known status from the Snapmaker U1. Each ToolHead can display:

- filament type
- color
- `Leer` / `Empty` when no filament is reported

The button background uses the reported filament color when available. The text color is chosen automatically as black or white, depending on which gives the best contrast.

### Reading a tag for sending

The `Tag lesen` action is independent of the currently selected QIDI/OpenSpool mode.

When a tag is placed on the reader, the firmware tries to identify the tag format automatically:

- QIDI tags are read as MIFARE Classic tag data
- OpenSpool tags are read as OpenSpool/NDEF data

After a valid tag is read, the decoded information is shown for 3 seconds. The device then returns to the ToolHead send menu so the user can choose the target ToolHead.

### Sending to a ToolHead

When a ToolHead is selected, the firmware first checks the target ToolHead filament sensor through a GET request.

If no filament is detected:

1. the tag data is sent directly
2. the result message is shown
3. the firmware returns to the ToolHead menu
4. the ToolHead status is queried again so the new value is visible

If filament is already detected:

1. a safety confirmation is shown
2. the user must explicitly confirm overwriting the ToolHead data
3. only then is the SET request sent
4. the ToolHead status is refreshed after the result screen

If the sensor state cannot be read, the firmware aborts the send action instead of silently overwriting data.

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

V4.2 uses a seven-page setup structure.

### Page 1/7: Core setup

Depending on mode and whether an official QIDI CFG is active, page 1 contains:

- manufacturer editor
- material editor
- language selection
- default startup mode
- active QIDI printer model selector

If an official QIDI CFG is active for the current model, manual QIDI material/manufacturer editing is intentionally hidden for that model.

### Page 2/7: Screensaver and brightness

Page 2 contains:

- screensaver timeout
- display brightness

Screensaver choices:

- `30 seconds`
- `1 minute`
- `5 minutes`
- `10 minutes`
- `Off`

### Page 3/7

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

Tapping a model row opens an info dialog explaining:

- that the user should obtain `officiall_filas_list.cfg` from Klipper
- that it can be copied manually to MicroSD
- or uploaded from a web browser when Wi-Fi is connected

#### OpenSpool mode page 3

Page 3 opens the OpenSpool tag information configuration pages:

- `OpenSpool Standard`
- `OpenSpool Extended`

### Page 4/7: Wi-Fi

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

### Page 5/7: Snapmaker U1

The Snapmaker U1 setup page stores:

- Snapmaker host or IP address
- Snapmaker API port

The default port is `7125`.

These values are used by the `Tag senden` workflow for ToolHead status GET requests, filament-sensor checks, and filament-data SET requests.

### Page 6/7: SD tools

The SD tools page contains:

- `Show SD card content`
- `Format SD card`
- live MicroSD status information

### Page 7/7: Device settings

The final page contains:

- display inversion
- touch calibration
- factory defaults

---

## QIDI official CFG support

V4.x adds per-model support for the official QIDI material list file:

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

V4.2 includes multiple input keyboards chosen automatically by field type.

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

V4.2 stores persistent settings in ESP32 preferences, including:

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
- Snapmaker U1 host
- Snapmaker U1 port
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

### Send a tag to Snapmaker U1

1. Open `Setup > Wi-Fi`.
2. Enable Wi-Fi and connect BoxRFID to the same network as the Snapmaker U1.
3. Open the Snapmaker U1 setup page.
4. Enter the Snapmaker host/IP and port.
5. Return to the main menu.
6. Tap `Tag senden`.
7. Tap `Tag lesen`.
8. Place a QIDI or OpenSpool tag on the reader.
9. Wait for the 3 second tag information popup.
10. Select the target ToolHead.
11. Confirm the overwrite warning only if filament is already detected in that ToolHead.
12. Check the refreshed ToolHead menu to verify the sent filament type and color.

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
5. Choose a variant only when it should be written to the tag.
6. Select `None` / `Keine` to remove the variant again. In that case the tag is written without a subtype field.
7. Review the slicer profile preview.
8. Tap `Write`.
9. Place the NTAG tag on the reader.

## Arduino IDE settings

The web installer is the preferred installation method. Manual Arduino IDE builds are useful for development and troubleshooting.

Recommended Arduino IDE settings for V4.2:

- Board: `ESP32-2432S028R CYD`
- CPU Frequency: `240MHz (WiFi/BT)`
- Flash Frequency: `80MHz`
- Flash Mode: `QIO`
- Flash Size: `4MB`
- Partition Scheme: `Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)`
- Arduino Runs On: `Core 1`
- Events Run On: `Core 1`
- Core Debug Level: `None`
- Zigbee Mode: `Disabled`

Upload notes:

- `921600` can work, but some USB adapters or cables are unstable at that speed.
- If upload fails with a stopped-responding error, lower upload speed to `460800` or `115200`.
- Use a data-capable USB cable.
- If the board is not detected, reconnect it and hold `BOOT` while connecting.

---

## Summary

BoxRFID OpenSpool Edition V4.2 is the current release for users who want:

- QIDI and OpenSpool in one firmware
- support for QIDI `Q2`, `Plus 4`, and `Max 4`
- official QIDI CFG file support through MicroSD and browser upload
- persistent Wi-Fi configuration
- direct Snapmaker U1 ToolHead sending
- live ToolHead status and filament-sensor overwrite protection
- improved keyboard layouts and setup flow
- editable local OpenSpool databases
- a fallback path back to internal QIDI lists when official CFG use is disabled

It is the recommended release for new installations.
