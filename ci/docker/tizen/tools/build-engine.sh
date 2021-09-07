#!/usr/bin/env bash
# Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -e

BUILD_MODE=$1
BUILD_ARCH=$2
BUILD_TRIPLE=$3

if [[ -z "$BUILD_MODE" || -z "$BUILD_ARCH" || -z "$BUILD_TRIPLE" ]]; then
    echo "Usage: $(basename "$0") <mode> <arch> <triple>"
    exit 1
fi

if [[ -z "$TIZEN_TOOLS_PATH" ]]; then
    TIZEN_TOOLS_PATH=/tizen_tools
fi
if [ ! -d "$TIZEN_TOOLS_PATH" ]; then
    echo "No such directory: $TIZEN_TOOLS_PATH"
    exit 1
fi

# FIXME: Remove unsupported options in BUILD.gn.
sed -i 's/"-Wno-non-c-typedef-for-linkage",//g' src/build/config/compiler/BUILD.gn
sed -i 's/"-Wno-psabi",//g' src/build/config/compiler/BUILD.gn

# Run gn.
src/flutter/tools/gn \
    --target-os linux \
    --linux-cpu $BUILD_ARCH \
    --no-goma \
    --target-toolchain "$TIZEN_TOOLS_PATH"/toolchains \
    --target-sysroot "$TIZEN_TOOLS_PATH"/sysroot/$BUILD_ARCH \
    --target-triple $BUILD_TRIPLE \
    --runtime-mode $BUILD_MODE \
    --enable-fontconfig \
    --embedder-for-target \
    --disable-desktop-embeddings \
    --build-tizen-shell

# Run ninja.
ninja -C src/out/linux_${BUILD_MODE}_${BUILD_ARCH}
