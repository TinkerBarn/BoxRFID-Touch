# macOS Arduino Development

This project builds with Arduino IDE's bundled `arduino-cli` on macOS.

## Required local setup

- Arduino IDE installed in `/Applications/Arduino IDE.app`
- ESP32 board package installed: `esp32:esp32`
- Board FQBN: `esp32:esp32:jczn_2432s028r`
- Arduino sketchbook containing the required libraries

Default sketchbook discovery looks for:

```text
~/Library/CloudStorage/OneDrive-Personlich/Dokumente/Arduino
```

The actual OneDrive folder may contain a localized or Unicode-normalized spelling. The scripts discover it automatically. You can override it explicitly:

```bash
export BOXRFID_SKETCHBOOK="/path/to/Arduino"
```

## Check the environment

```bash
tools/check-macos-arduino.sh
```

Required libraries:

- `TFT_eSPI`
- `XPT2046_Touchscreen`
- `Adafruit_PN532`
- `Adafruit_BusIO`
- `ArduinoJson`

## Compile

Compile the current development sketch:

```bash
tools/compile-arduino.sh
```

Compile a specific sketch:

```bash
tools/compile-arduino.sh source/boxrfid-openspool/v4.1/BoxRFID_CYD_ESP32_2432S028R_Qidi_OpenSpool_V4.1.ino
```

Build output is written below `.build/`.

## Ports and upload

List connected boards and serial ports:

```bash
tools/list-arduino-ports.sh
```

Upload a previously compiled sketch, compiling first if needed:

```bash
tools/upload-arduino.sh /dev/cu.usbserial-XXXX
```

## Apple Silicon note

The Arduino builtin `ctags` package for macOS is currently x86_64. On Apple Silicon, Rosetta must be installed:

```bash
softwareupdate --install-rosetta --agree-to-license
```
