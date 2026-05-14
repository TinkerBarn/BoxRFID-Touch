#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

export BOXRFID_REPO_ROOT="${BOXRFID_REPO_ROOT:-$repo_root}"
export BOXRFID_FQBN="${BOXRFID_FQBN:-esp32:esp32:jczn_2432s028r}"
export BOXRFID_BUILD_ROOT="${BOXRFID_BUILD_ROOT:-$BOXRFID_REPO_ROOT/.build}"
export BOXRFID_ARDUINO_DATA_DIR="${BOXRFID_ARDUINO_DATA_DIR:-$HOME/Library/Arduino15}"

if [[ -z "${BOXRFID_ARDUINO_CLI:-}" ]]; then
  if command -v arduino-cli >/dev/null 2>&1; then
    BOXRFID_ARDUINO_CLI="$(command -v arduino-cli)"
  elif [[ -x "/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli" ]]; then
    BOXRFID_ARDUINO_CLI="/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli"
  else
    BOXRFID_ARDUINO_CLI=""
  fi
fi
export BOXRFID_ARDUINO_CLI

resolve_sketchbook() {
  if [[ -n "${BOXRFID_SKETCHBOOK:-}" ]]; then
    printf '%s\n' "$BOXRFID_SKETCHBOOK"
    return 0
  fi

  local cloud="$HOME/Library/CloudStorage"
  local candidate
  candidate="$(find "$cloud" -maxdepth 1 -type d -name 'OneDrive-Pers*nlich' -print 2>/dev/null | head -n 1 || true)"
  if [[ -n "$candidate" && -d "$candidate/Dokumente/Arduino" ]]; then
    printf '%s\n' "$candidate/Dokumente/Arduino"
    return 0
  fi

  if [[ -d "$HOME/Documents/Arduino" ]]; then
    printf '%s\n' "$HOME/Documents/Arduino"
    return 0
  fi

  printf '%s\n' ""
}

export BOXRFID_SKETCHBOOK="$(resolve_sketchbook)"
export BOXRFID_LIBRARIES_DIR="${BOXRFID_LIBRARIES_DIR:-$BOXRFID_SKETCHBOOK/libraries}"
