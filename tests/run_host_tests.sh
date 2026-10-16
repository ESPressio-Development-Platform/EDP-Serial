#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output="$(mktemp)"
trap 'rm -f "$output"' EXIT
system="${EDP_SYSTEM_SOURCE_DIR:-$root/../EDP-System}"
test -d "$system/src" || { echo "EDP-System source is required" >&2; exit 2; }
"${CXX:-c++}" -std=c++20 -Wall -Wextra -Wpedantic -Werror -I"$root/src" -I"$system/src" "$root/tests/ByteEndpointTests.cpp" -o "$output"
"$output"
echo 'EDP-Serial bounded abstract endpoint: PASS'
