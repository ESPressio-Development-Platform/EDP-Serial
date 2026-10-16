#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SYSTEM="${EDP_SYSTEM_SOURCE_DIR:-$ROOT/../EDP-System}"
LOGGING="${EDP_LOGGING_SOURCE_DIR:-$ROOT/../EDP-Logging}"
OUTPUT="$(mktemp)"
trap 'rm -f "$OUTPUT"' EXIT
test -d "$SYSTEM/src" || { echo "Missing EDP-System" >&2; exit 2; }
test -d "$LOGGING/src" || { echo "Missing EDP-Logging" >&2; exit 2; }
"${CXX:-c++}" -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  -I"$ROOT/src" -I"$SYSTEM/src" -I"$LOGGING/src" \
  "$ROOT/tests/EdlgFramePumpTests.cpp" -o "$OUTPUT"
"$OUTPUT"
echo "EDP-Serial opt-in complete EDLG frame partial-write pump: PASS"
