#!/bin/sh
# Copy common/rt.h next to every sketch. The Arduino IDE only sees headers
# that sit in the sketch folder, so each lab carries a copy of the master.
# Run after every change to common/rt.h. With --check, only report copies
# that differ from the master (used by `make compile`). Sketches that do not
# include rt.h (Lab 0, plain Arduino) get no copy.
set -e
cd "$(dirname "$0")/.."
status=0
for ino in lab*/*/*.ino; do
    dir=$(dirname "$ino")
    grep -q '#include "rt.h"' "$ino" || continue
    if [ "$1" = "--check" ]; then
        if ! cmp -s common/rt.h "$dir/rt.h"; then
            echo "out of date: $dir/rt.h (run tools/sync_common.sh)"
            status=1
        fi
    else
        cp common/rt.h "$dir/rt.h"
    fi
done
exit $status
