#!/bin/sh
# Copy common/rtlab.h next to every sketch. The Arduino IDE only sees headers
# that sit in the sketch folder, so each lab carries a copy of the master.
# Run after every change to common/rtlab.h. With --check, only report copies
# that differ from the master (used by `make compile`).
set -e
cd "$(dirname "$0")/.."
status=0
for ino in lab*/*/*.ino; do
    dir=$(dirname "$ino")
    if [ "$1" = "--check" ]; then
        if ! cmp -s common/rtlab.h "$dir/rtlab.h"; then
            echo "out of date: $dir/rtlab.h (run tools/sync_common.sh)"
            status=1
        fi
    else
        cp common/rtlab.h "$dir/rtlab.h"
    fi
done
exit $status
