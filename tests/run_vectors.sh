#!/usr/bin/env bash
# Bit-exact regression against the ITU test vectors.
# Requires a build with -DG729A_ITU_TEST_FORMAT=ON (164 bytes/frame format).
#
# Usage: bash tests/run_vectors.sh [build_dir]
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-$ROOT/build}"
TV="$ROOT/test_vectors"

CODER="$(find "$BUILD_DIR" -maxdepth 3 -name 'coder' -o -maxdepth 3 -name 'coder.exe' | head -1)"
DECODER="$(find "$BUILD_DIR" -maxdepth 3 -name 'decoder' -o -maxdepth 3 -name 'decoder.exe' | head -1)"

if [ -z "$CODER" ] || [ -z "$DECODER" ]; then
    echo "ERROR: coder/decoder binaries not found under $BUILD_DIR" >&2
    exit 1
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

fail=0

# Encoder: IN -> BIT
for in_file in "$TV/IN"/*.IN; do
    name="$(basename "$in_file" .IN)"
    ref="$TV/BIT/$name.BIT"
    if [ ! -f "$ref" ]; then
        echo "SKIP  enc $name (no reference BIT)"
        continue
    fi
    "$CODER" "$in_file" "$TMP/$name.BIT" >/dev/null 2>&1
    if cmp -s "$TMP/$name.BIT" "$ref"; then
        echo "PASS  enc $name"
    else
        echo "FAIL  enc $name"
        fail=1
    fi
done

# Decoder: BIT -> PST
for bit_file in "$TV/BIT"/*.BIT; do
    name="$(basename "$bit_file" .BIT)"
    ref="$TV/PST/$name.PST"
    if [ ! -f "$ref" ]; then
        echo "SKIP  dec $name (no reference PST)"
        continue
    fi
    "$DECODER" "$bit_file" "$TMP/$name.PST" >/dev/null 2>&1
    if cmp -s "$TMP/$name.PST" "$ref"; then
        echo "PASS  dec $name"
    else
        echo "FAIL  dec $name"
        fail=1
    fi
done

if [ "$fail" -ne 0 ]; then
    echo "RESULT: FAIL" >&2
    exit 1
fi
echo "RESULT: PASS"
