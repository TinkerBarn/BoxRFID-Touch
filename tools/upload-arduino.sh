#!/usr/bin/env bash
set -euo pipefail

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/arduino-env.sh"

if [[ $# -lt 1 ]]; then
  printf 'Usage: %s /dev/cu.usbserial-XXXX [sketch.ino]\n' "$(basename "$0")" >&2
  printf 'Ports:\n' >&2
  "$BOXRFID_REPO_ROOT/tools/list-arduino-ports.sh" >&2
  exit 2
fi

port="$1"
sketch="${2:-$BOXRFID_REPO_ROOT/dev/boxrfid-openspool/BoxRFID_CYD_ESP32_2432S028R_Qidi_OpenSpool_V4.1.1.ino}"
sketch_abs="$(cd "$(dirname "$sketch")" && pwd)/$(basename "$sketch")"
sketch_name="$(basename "$sketch_abs" .ino)"
stage_dir="$BOXRFID_BUILD_ROOT/sketches/$sketch_name"
build_dir="$BOXRFID_BUILD_ROOT/$sketch_name"

if [[ ! -d "$build_dir" ]]; then
  "$BOXRFID_REPO_ROOT/tools/compile-arduino.sh" "$sketch_abs"
fi

"$BOXRFID_ARDUINO_CLI" upload \
  --fqbn "$BOXRFID_FQBN" \
  --port "$port" \
  --input-dir "$build_dir/out" \
  "$stage_dir"
