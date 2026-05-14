#!/usr/bin/env bash
set -euo pipefail

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/arduino-env.sh"

default_sketch="$BOXRFID_REPO_ROOT/dev/boxrfid-openspool/BoxRFID_CYD_ESP32_2432S028R_Qidi_OpenSpool_V4.1.1.ino"
sketch="${1:-$default_sketch}"

if [[ -z "$BOXRFID_ARDUINO_CLI" || ! -x "$BOXRFID_ARDUINO_CLI" ]]; then
  printf 'arduino-cli not found. Install Arduino IDE or set BOXRFID_ARDUINO_CLI.\n' >&2
  exit 1
fi

if [[ ! -f "$sketch" ]]; then
  printf 'Sketch not found: %s\n' "$sketch" >&2
  exit 1
fi

if [[ ! -d "$BOXRFID_LIBRARIES_DIR" ]]; then
  printf 'Arduino libraries dir not found: %s\n' "$BOXRFID_LIBRARIES_DIR" >&2
  printf 'Set BOXRFID_SKETCHBOOK or BOXRFID_LIBRARIES_DIR.\n' >&2
  exit 1
fi

if ! arch -x86_64 /usr/bin/true >/dev/null 2>&1; then
  printf 'Rosetta is missing. Arduino ctags is x86_64 on macOS and compile cannot start.\n' >&2
  printf 'Run: softwareupdate --install-rosetta --agree-to-license\n' >&2
  exit 1
fi

sketch_abs="$(cd "$(dirname "$sketch")" && pwd)/$(basename "$sketch")"
sketch_name="$(basename "$sketch_abs" .ino)"
source_dir="$(dirname "$sketch_abs")"
stage_dir="$BOXRFID_BUILD_ROOT/sketches/$sketch_name"
build_dir="$BOXRFID_BUILD_ROOT/$sketch_name"
out_dir="$build_dir/out"

mkdir -p "$stage_dir" "$build_dir" "$out_dir"
find "$stage_dir" -mindepth 1 -maxdepth 1 -exec rm -rf {} +

while IFS= read -r item; do
  cp -R "$item" "$stage_dir/"
done < <(find "$source_dir" -mindepth 1 -maxdepth 1 -print)

if [[ ! -f "$stage_dir/$sketch_name.ino" ]]; then
  mv "$stage_dir/$(basename "$sketch_abs")" "$stage_dir/$sketch_name.ino"
fi

printf 'Compiling %s\n' "$sketch_abs"
printf 'Board: %s\n' "$BOXRFID_FQBN"
printf 'Libraries: %s\n' "$BOXRFID_LIBRARIES_DIR"

"$BOXRFID_ARDUINO_CLI" compile \
  --fqbn "$BOXRFID_FQBN" \
  --libraries "$BOXRFID_LIBRARIES_DIR" \
  --build-path "$build_dir" \
  --output-dir "$out_dir" \
  --export-binaries \
  "$stage_dir"

printf '\nBuild output:\n%s\n' "$out_dir"
