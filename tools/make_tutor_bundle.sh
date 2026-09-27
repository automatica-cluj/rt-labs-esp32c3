#!/bin/sh
# Build tutor/tutor_bundle.md: the assistant instructions (tutor/rules.md)
# followed by every lab README, the docs, common/rt.h and the source of every
# sketch. Students attach the file to a chat, see tutor/README.md.
# Run after changing any of those files. With --check, only report whether
# the committed bundle is out of date.
set -e
cd "$(dirname "$0")/.."
out=tutor/tutor_bundle.md

# One file, wrapped so the assistant can tell where it starts and ends.
emit() {
    printf '\n\n===== FILE: %s =====\n\n' "$1"
    case "$1" in
        *.md) cat "$1" ;;
        *) printf '```cpp\n'; cat "$1"; printf '```\n' ;;
    esac
}

build() {
    cat tutor/rules.md
    emit README.md
    for f in docs/*.md; do emit "$f"; done
    emit common/rt.h
    for lab in lab*/; do
        emit "${lab}README.md"
        for ino in "$lab"*/*.ino; do emit "$ino"; done
    done
}

if [ "$1" = "--check" ]; then
    tmp=$(mktemp)
    build > "$tmp"
    if cmp -s "$tmp" "$out"; then status=0; else
        echo "out of date: $out (run tools/make_tutor_bundle.sh)"
        status=1
    fi
    rm -f "$tmp"
    exit $status
fi

build > "$out"
echo "$out: $(wc -c < "$out" | tr -d ' ') bytes"
