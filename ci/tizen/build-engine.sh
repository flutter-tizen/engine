#!/usr/bin/env bash
# Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -e

BUILD_MODE=debug

while [ $# -ne 0 ]; do
    name=$1
    case "$name" in
    -a | --target-arch)
        shift; BUILD_ARCH=$1
        ;;
    -m | --runtime-mode)
        shift; BUILD_MODE=$1
        ;;
    -t | --target-triple)
        shift; BUILD_TRIPLE=$1
        ;;
    *)
        BUILD_TARGET="$@"
        break
    esac
    shift
done

if [[ -z "$TIZEN_TOOLS_PATH" ]]; then
    TIZEN_TOOLS_PATH=/tizen_tools
fi
if [ ! -d "$TIZEN_TOOLS_PATH" ]; then
    echo "No such directory: $TIZEN_TOOLS_PATH"
    exit 1
fi

if [[ -z "$BUILD_ARCH" || -z "$BUILD_TRIPLE" ]]; then
    echo "Required arguments are missing."
    exit 1
fi

SCRIPT_DIR="$(dirname "${BASH_SOURCE[0]}")"
cd "$SCRIPT_DIR"/../../..

# FIXME: Remove unsupported options of tizen toolchains from BUILD.gn.
sed -i 's/"-Wno-non-c-typedef-for-linkage",//g' build/config/compiler/BUILD.gn
sed -i 's/"-Wno-psabi",//g' build/config/compiler/BUILD.gn
sed -i 's/"-Wno-unused-but-set-parameter",//g' build/config/compiler/BUILD.gn
sed -i 's/"-Wno-unused-but-set-variable",//g' build/config/compiler/BUILD.gn

flutter/tools/gn \
    --target-os linux \
    --linux-cpu $BUILD_ARCH \
    --no-goma \
    --target-toolchain "$TIZEN_TOOLS_PATH"/toolchains \
    --target-sysroot "$TIZEN_TOOLS_PATH"/sysroot/$BUILD_ARCH \
    --target-triple $BUILD_TRIPLE \
    --runtime-mode $BUILD_MODE \
    --enable-fontconfig \
    --no-full-dart-sdk \
    --no-build-embedder-examples \
    --embedder-for-target \
    --build-tizen-shell
ninja -C out/linux_${BUILD_MODE}_${BUILD_ARCH} ${BUILD_TARGET}
