#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 /path/to/patched/diskscan-sources" >&2
    exit 2
fi

source_dir=$(CDPATH= cd -- "$1" && pwd -P)
test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT HUP INT TERM
ln -s "$source_dir/arch/arch-linux.h" "$work_dir/arch-internal.h"

"${CC:-cc}" ${CFLAGS:-} -std=gnu99 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 \
    -UNDEBUG -ffunction-sections -fdata-sections -I"$work_dir" \
    -I"$source_dir/include" -I"$source_dir" -I"$source_dir/libscsicmd/include" \
    "$test_dir/smart-monitor.c" "$source_dir/lib/disk.c" \
    "$source_dir/arch/arch-linux.c" "$source_dir"/libscsicmd/src/*.c \
    "$source_dir"/libscsicmd/src/smartdb/*.c \
    -Wl,--gc-sections -Wl,--wrap=ioctl -o "$work_dir/smart-monitor"

${RUNNER:-} "$work_dir/smart-monitor"
