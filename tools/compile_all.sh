#!/bin/sh
# Compile every sketch for the ESP32-C3 with all warnings on. No board needed.
# Uses arduino-cli from PATH, or the copy inside the Arduino IDE on macOS.
set -e
cd "$(dirname "$0")/.."
CLI=${ARDUINO_CLI:-$(command -v arduino-cli || true)}
MAC_CLI="/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli"
[ -z "$CLI" ] && [ -x "$MAC_CLI" ] && CLI=$MAC_CLI
[ -z "$CLI" ] && { echo "arduino-cli not found; set ARDUINO_CLI"; exit 1; }
FQBN=${FQBN:-esp32:esp32:esp32c3:CDCOnBoot=cdc}
# A private sketchbook dir: keeps ~/Documents/Arduino out of it (on macOS the
# CLI can hang there on the privacy prompt) and makes builds reproducible.
export ARDUINO_DIRECTORIES_USER=${ARDUINO_DIRECTORIES_USER:-$PWD/.arduino-user}
mkdir -p "$ARDUINO_DIRECTORIES_USER"

tools/sync_common.sh --check
fail=0
for ino in lab*/*/*.ino; do
    dir=$(dirname "$ino")
    printf '%-45s ' "$dir"
    if out=$("$CLI" compile --fqbn "$FQBN" --warnings all "$dir" 2>&1); then
        # The core's own headers warn; only our files have to be clean.
        if echo "$out" | grep -qE '(\.ino|rt\.h):[0-9]+:[0-9]+: warning'; then
            echo "WARNINGS"; echo "$out" | grep -E -A3 '(\.ino|rt\.h):[0-9]+:[0-9]+: warning'; fail=1
        else
            echo "$out" | grep -o 'Sketch uses [0-9]* bytes ([0-9]*%)' || echo ok
        fi
    else
        echo "FAILED"; echo "$out" | tail -30; fail=1
    fi
done
exit $fail
