# Version History

This file is the ongoing release history for the public BoxRFID firmware lines.

## BoxRFID OpenSpool Edition V4.2

- Status: Current release
- Firmware: [firmware/boxrfid-openspool/v4.2/](../firmware/boxrfid-openspool/v4.2/)
- Source: [source/boxrfid-openspool/v4.2/](../source/boxrfid-openspool/v4.2/)

Highlights:

- direct `Tag senden` workflow for Snapmaker U1 ToolHeads
- ToolHead send menu opens directly and uses a localized title
- localized read-tag action in the ToolHead send menu
- QIDI and OpenSpool tags are detected automatically when reading a tag for sending, independent of the currently selected firmware mode
- tag information is shown for 3 seconds before the ToolHead send menu is displayed
- Snapmaker U1 ToolHead status is queried through GET requests and displayed with material type and color
- empty ToolHeads are shown as `Leer` / `Empty`
- ToolHead button text automatically switches between black and white for best contrast against the filament color
- before sending, the target ToolHead filament sensor is checked; if filament is detected, the firmware asks for confirmation before overwriting
- after a successful send, the ToolHead status is refreshed so the user can verify the result immediately
- Snapmaker host and port setup is stored persistently
- safer preference loading for user-edited material/manufacturer/variant lists and Wi-Fi/Snapmaker settings to avoid crashes after custom QIDI list edits
- OpenSpool U1 tag writing now clears the subtype correctly when `None` / `Keine` is selected again
- web installer changed to a release selector instead of separate install cards

## BoxRFID OpenSpool Edition V4.1

- Status: Stable previous release
- Firmware: [firmware/boxrfid-openspool/v4.1/](../firmware/boxrfid-openspool/v4.1/)
- Source: [source/boxrfid-openspool/v4.1/](../source/boxrfid-openspool/v4.1/)

Highlights:

- published release based on the V4.0.x development line as the new current version
- automatic backup of user setup and edited lists to MicroSD when a card is present
- automatic restore after firmware updates or device migration when the same MicroSD card is reused
- dedicated SD card tools page with card content view and formatting workflow
- improved web CFG interface with English UI, stored-file state, card-content listing, and direct CFG content viewing
- added mDNS access so the web interface can also be reached via `http://boxrfid.local`
- improved runtime MicroSD status handling with hotplug detection on the relevant setup pages
- continued fixes for touch alignment, OpenSpool defaults, Wi-Fi diagnostics, and general usability
- current combined standalone release for OpenSpool Standard, Snapmaker U1 with paxx12 Extended Firmware, and QIDI Box with Q2 / Plus 4 / Max 4

## BoxRFID OpenSpool Edition V4.0.1

- Status: Stable fallback
- Firmware: [firmware/boxrfid-openspool/v4.0.1/](../firmware/boxrfid-openspool/v4.0.1/)
- Source: [source/boxrfid-openspool/v4.0.1/](../source/boxrfid-openspool/v4.0.1/)

Highlights:

- improved Wi-Fi debugging directly on the device
- improved Wi-Fi icon handling and header integration
- fixed multiple OpenSpool write-field defaults after reading tags with missing optional fields
- fixed OpenSpool touch areas for temperature, HEX, weight, and diameter fields
- improved reconnect diagnostics, DHCP visibility, and live Wi-Fi status logging
- continued stability fixes for touch, SD card, Wi-Fi, and QIDI CFG workflows
- current combined standalone release for OpenSpool Standard, Snapmaker U1 with paxx12 Extended Firmware, and QIDI Box with Q2 / Plus 4 / Max 4

## BoxRFID OpenSpool Edition V4.0

- Status: Previous public release
- Firmware: [firmware/boxrfid-openspool/v4.0/](../firmware/boxrfid-openspool/v4.0/)
- Source: [source/boxrfid-openspool/v4.0/](../source/boxrfid-openspool/v4.0/)

Highlights:

- added QIDI Max 4 support alongside QIDI Q2 and QIDI Plus 4 for QIDI Box workflows
- added persistent Wi-Fi setup with stored SSID and password
- added browser upload for `officiall_filas_list.cfg`
- added MicroSD-based official QIDI CFG handling for Q2, Plus 4, and Max 4
- added per-model CFG activation toggles in setup
- added SD card formatting with automatic QIDI folder structure creation
- improved keyboards, setup flow, and overall usability

## BoxRFID OpenSpool Edition V3.7

- Status: Older combined release
- Firmware: [firmware/boxrfid-openspool/v3.7/](../firmware/boxrfid-openspool/v3.7/)
- Source: [source/boxrfid-openspool/v3.7/](../source/boxrfid-openspool/v3.7/)

Highlights:

- older public combined release for the QIDI + OpenSpool firmware line
- kept available in the web installer for compatibility and reference

## BoxRFID-Touch V2.1

- Status: Stable classic
- Firmware: [firmware/boxrfid-touch/v2.1/](../firmware/boxrfid-touch/v2.1/)
- Source: [source/boxrfid-touch/v2.1/](../source/boxrfid-touch/v2.1/)

Highlights:

- classic QIDI-only firmware line
- kept available in the web installer for users who do not need OpenSpool features
