#!/usr/bin/env bash
set -euo pipefail

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/arduino-env.sh"

if [[ -z "$BOXRFID_ARDUINO_CLI" || ! -x "$BOXRFID_ARDUINO_CLI" ]]; then
  printf 'arduino-cli not found. Install Arduino IDE or set BOXRFID_ARDUINO_CLI.\n' >&2
  exit 1
fi

"$BOXRFID_ARDUINO_CLI" board list
