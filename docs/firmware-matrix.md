# Firmware Matrix

This repository currently provides the following public firmware builds for the `ESP32-2432S028R (CYD)` platform.

| Firmware line | Version | Status | Intended use | Web Installer | Binary path |
| --- | --- | --- | --- | --- | --- |
| BoxRFID-Touch | V2.0 | Legacy | Older classic QIDI firmware | No | `firmware/boxrfid-touch/v2.0/BoxRFID_CYD_ESP32_2432S028R_V2.0.ino.merged.bin` |
| BoxRFID-Touch | V2.1 | Stable | Recommended classic QIDI firmware | Yes | `firmware/boxrfid-touch/v2.1/BoxRFID_CYD_ESP32_2432S028R_V2.1.ino.merged.bin` |
| BoxRFID OpenSpool Edition | V3.6 | Stable | Recommended OpenSpool-capable firmware | Yes | `firmware/boxrfid-openspool/v3.6/BoxRFID_CYD_ESP32_2432S028R_Qidi_OpenSpool_V3.6.ino.merged.bin` |

## Notes

- `V2.1` is the recommended public build for the classic QIDI workflow.
- `V3.6` is the recommended public build for users who need OpenSpool support.
- `V2.0` remains available in the repository as a legacy binary for older setups.
- Arduino source files are currently maintained locally and are not part of the public GitHub repository.
