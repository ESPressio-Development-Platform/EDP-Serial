#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORKSPACE="${EDP_SERIAL_DEPENDENCY_ROOT:-$(cd "$ROOT/.." && pwd)}"
LOGGING="${EDP_LOGGING_SOURCE_DIR:-$WORKSPACE/EDP-Logging}"
OUTPUT="$(mktemp)"
trap 'rm -f "$OUTPUT"' EXIT
for repo in EDP-System EDP-Platform EDP-Clock EDP-Threading EDP-BoundedTopology EDP-Memory EDP-Platform-Portable; do
    test -d "$WORKSPACE/$repo/src" || { echo "Missing $repo" >&2; exit 2; }
done
test -d "$LOGGING/src" || { echo "Missing EDP-Logging" >&2; exit 2; }

"${CXX:-c++}" -std=c++20 -pthread -Wall -Wextra -Wpedantic -Werror \
    -I"$ROOT/src" -I"$LOGGING/src" \
    -I"$WORKSPACE/EDP-System/src" \
    -I"$WORKSPACE/EDP-Platform/src" \
    -I"$WORKSPACE/EDP-Clock/src" \
    -I"$WORKSPACE/EDP-Threading/src" \
    -I"$WORKSPACE/EDP-BoundedTopology/src" \
    -I"$WORKSPACE/EDP-Memory/src" -I"$WORKSPACE/EDP-Platform-Portable/src" \
    "$ROOT/tests/BoundedEdlgFrameQueueTests.cpp" -o "$OUTPUT"
"$OUTPUT"
echo "EDP-Serial opt-in composed FIFO EDLG queue: PASS"
