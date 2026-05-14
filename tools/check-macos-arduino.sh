#!/usr/bin/env bash
set -euo pipefail

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/arduino-env.sh"

failures=0

section() {
  printf '\n== %s ==\n' "$1"
}

ok() {
  printf 'OK   %s\n' "$1"
}

warn() {
  printf 'WARN %s\n' "$1"
}

fail() {
  printf 'FAIL %s\n' "$1"
  failures=$((failures + 1))
}

section "System"
printf 'macOS arch: %s\n' "$(uname -m)"

if arch -x86_64 /usr/bin/true >/dev/null 2>&1; then
  ok "Rosetta can run x86_64 tools"
else
  fail "Rosetta is missing; Arduino ctags is x86_64 and cannot run on this Mac"
  printf '     Run: softwareupdate --install-rosetta --agree-to-license\n'
fi

section "Arduino CLI"
if [[ -n "$BOXRFID_ARDUINO_CLI" && -x "$BOXRFID_ARDUINO_CLI" ]]; then
  ok "arduino-cli found: $BOXRFID_ARDUINO_CLI"
  "$BOXRFID_ARDUINO_CLI" version || failures=$((failures + 1))
else
  fail "arduino-cli not found. Install Arduino IDE or put arduino-cli in PATH."
fi

section "Arduino Data"
if [[ -d "$BOXRFID_ARDUINO_DATA_DIR" ]]; then
  ok "Arduino data dir exists: $BOXRFID_ARDUINO_DATA_DIR"
else
  fail "Arduino data dir missing: $BOXRFID_ARDUINO_DATA_DIR"
fi

if [[ -n "$BOXRFID_ARDUINO_CLI" && -x "$BOXRFID_ARDUINO_CLI" ]]; then
  if "$BOXRFID_ARDUINO_CLI" core list | grep -q '^esp32:esp32'; then
    ok "ESP32 core is installed"
  else
    fail "ESP32 core is not installed"
    printf '     Run: "%s" core install esp32:esp32\n' "$BOXRFID_ARDUINO_CLI"
  fi

  if "$BOXRFID_ARDUINO_CLI" board listall esp32 | grep -q 'esp32:esp32:jczn_2432s028r'; then
    ok "Board FQBN exists: esp32:esp32:jczn_2432s028r"
  else
    fail "Board FQBN missing: esp32:esp32:jczn_2432s028r"
  fi
fi

section "Sketchbook"
if [[ -n "$BOXRFID_SKETCHBOOK" && -d "$BOXRFID_SKETCHBOOK" ]]; then
  ok "Sketchbook found: $BOXRFID_SKETCHBOOK"
else
  fail "Sketchbook not found. Set BOXRFID_SKETCHBOOK to your Arduino sketchbook path."
fi

if [[ -d "$BOXRFID_LIBRARIES_DIR" ]]; then
  ok "Libraries dir found: $BOXRFID_LIBRARIES_DIR"
else
  fail "Libraries dir missing: $BOXRFID_LIBRARIES_DIR"
fi

required_libraries=(
  "TFT_eSPI"
  "XPT2046_Touchscreen"
  "Adafruit_PN532"
  "Adafruit_BusIO"
  "ArduinoJson"
)

for lib in "${required_libraries[@]}"; do
  if [[ -f "$BOXRFID_LIBRARIES_DIR/$lib/library.properties" ]]; then
    ok "Library installed: $lib"
  else
    fail "Library missing: $lib"
  fi
done

if [[ -w "$BOXRFID_REPO_ROOT" ]]; then
  ok "Repo is writable: $BOXRFID_REPO_ROOT"
else
  fail "Repo is not writable: $BOXRFID_REPO_ROOT"
fi

if [[ -w "$BOXRFID_BUILD_ROOT" || ! -e "$BOXRFID_BUILD_ROOT" && -w "$BOXRFID_REPO_ROOT" ]]; then
  ok "Build output can be written under: $BOXRFID_BUILD_ROOT"
else
  fail "Build output is not writable: $BOXRFID_BUILD_ROOT"
fi

if [[ -n "$BOXRFID_SKETCHBOOK" && -d "$BOXRFID_SKETCHBOOK" ]]; then
  if [[ -w "$BOXRFID_SKETCHBOOK" ]]; then
    ok "Sketchbook is writable"
  else
    warn "Sketchbook is not writable from this process; compiling only needs read access"
  fi
fi

section "Result"
if [[ "$failures" -eq 0 ]]; then
  ok "Environment looks ready"
else
  printf 'FAIL %s required check(s) failed\n' "$failures"
fi

exit "$failures"
