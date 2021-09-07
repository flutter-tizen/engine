#!/usr/bin/env bash
# Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# CACHE SCHECKSUM #
#
# Ninja build uses the modification time (mtime) of files for the incremental
# build. After `gclient sync`, however, the incremental build does not work
# because all synchronized files are newly updated and the mtime of files will
# be newer than the cached out files.
# So it is necessary to record the checksum of all source files used for the
# cache output, and restore the mtime for the changed files using this checksum.
#
# Usages: cache-checksum <restore|save> <cache-dir>
#
# save: records the checksums of all files in TARGET_DIRS.
# restore: finds the changed files using the recorded checksums, and make the
#          files to have a new mtime than the cached files.
#

set -e

TARGET_DIRS="src/flutter src/third_party src/buildtools"

COMMAND=$1
CACHE_PATH=$2

if [[ -z "$COMMAND" || -z "$CACHE_PATH" ]]; then
    echo "Usage: $(basename "$0") <restore|save> <cache-dir>"
    exit 1
fi

CHECKSUM_FILE=$CACHE_PATH/.checksum.log

if [[ "$COMMAND" == "restore" ]]; then
    if [ ! -f "$CHECKSUM_FILE" ]; then
        echo "No $CHECKSUM_FILE file. skipped."
        exit 0
    fi

    # Set mtime of files in $TARGET_DIRS to OLD time.
    for d in $(find $TARGET_DIRS -type d -not -path "*/.git*"); do
        touch -c -m -d @1600000000 $d/* &
    done

    # Set mtime of changed files to NEW time.
    for f in $(sha1sum -c --quiet $CHECKSUM_FILE 2>/dev/null | grep FAILED | cut -d: -f1); do
        if [ -f $f ]; then
            echo "changed file: $f"
            touch $f
        fi
    done

elif [[ "$COMMAND" == "save" ]]; then
    find $TARGET_DIRS -type f -not -path "*/.git*" -print0 | xargs -r0 sha1sum >$CHECKSUM_FILE
else
    echo "unknown command: $COMMAND"
    exit 1
fi
