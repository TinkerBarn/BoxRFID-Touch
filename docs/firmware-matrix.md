# Firmware Matrix

This repository provides public firmware lines for the `ESP32-2432S028R (CYD)` platform.

| Firmware line | Version | Status | Intended use | Web Installer | Binary path |
| --- | --- | --- | --- | --- | --- |
| BoxRFID-Touch | V2.0 | Legacy | Older classic QIDI firmware kept for compatibility | No | `firmware/boxrfid-touch/v2.0/BoxRFID_CYD_ESP32_2432S028R_V2.0.ino.merged.bin` |
| BoxRFID-Touch | V2.1 | Stable classic | Classic QIDI-only workflow | Yes | `firmware/boxrfid-touch/v2.1/BoxRFID_CYD_ESP32_2432S028R_V2.1.ino.merged.bin` |
| BoxRFID OpenSpool Edition | V4.0.1 | Stable fallback | Previous QIDI plus OpenSpool release kept as fallback | Yes | `firmware/boxrfid-openspool/v4.0.1/BoxRFID_CYD_ESP32_2432S028R_Qidi_OpenSpool_V4.0.1.ino.merged.bin` |
| BoxRFID OpenSpool Edition | V3.7 | Older combined release | Earlier combined release kept for compatibility | Yes | `firmware/boxrfid-openspool/v3.7/BoxRFID_CYD_ESP32_2432S028R_Qidi_OpenSpool_V3.7.ino.merged.bin` |
| BoxRFID OpenSpool Edition | V4.1 | Current release | Latest QIDI plus OpenSpool release with Q2, Plus 4, Max 4, Wi-Fi CFG upload, SD backup/restore, improved web CFG tools, and setup improvements | Yes | `firmware/boxrfid-openspool/v4.1/BoxRFID_CYD_ESP32_2432S028R_Qidi_OpenSpool_V4.1.ino.merged.bin` |

## Release guidance

- Use `V4.1` if you want the latest release and all current QIDI/OpenSpool features.
- Use `V4.0.1` if you need the previous combined firmware as a fallback.
- Use `V3.7` only if you specifically need the older combined release.
- Use `V2.1` if you only want the classic QIDI firmware line.
- `V2.0` remains available only for older setups.

## Source paths

- Classic QIDI source: `source/boxrfid-touch/v2.1/`
- OpenSpool fallback source: `source/boxrfid-openspool/v4.0.1/`
- Older combined OpenSpool source: `source/boxrfid-openspool/v3.7/`
- OpenSpool V4.0 source: `source/boxrfid-openspool/v4.0/`
- OpenSpool V4.0.1 source: `source/boxrfid-openspool/v4.0.1/`
- Current OpenSpool source: `source/boxrfid-openspool/v4.1/`
