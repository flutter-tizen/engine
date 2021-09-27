#!/usr/bin/env bash
# Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -e

BUILD_MODE=debug
BUILD_OS=host

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
    -o | --target-os)
        shift; BUILD_OS=$1
        ;;
    --build-target)
        shift; BUILD_TARGET=$1
        ;;
    *)
        echo "Unknown argument \`$name\`"
        exit 1
        ;;
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

if [[ "$BUILD_OS" == "host" ]]; then
    src/flutter/tools/gn \
        --no-goma \
        --runtime-mode $BUILD_MODE \
        --enable-fontconfig \
        --build-tizen-shell
    ninja -C src/out/${BUILD_OS}_${BUILD_MODE} ${BUILD_TARGET}
else
    if [[ -z "$BUILD_ARCH" || -z "$BUILD_TRIPLE" ]]; then
        echo "required arguments are missing."
        exit 1
    fi

    # FIXME: Remove unsupported options of tizen toolchains from BUILD.gn.
    sed -i 's/"-Wno-non-c-typedef-for-linkage",//g' src/build/config/compiler/BUILD.gn
    sed -i 's/"-Wno-psabi",//g' src/build/config/compiler/BUILD.gn
    sed -i 's/"-Wno-unused-but-set-parameter",//g' build/config/compiler/BUILD.gn
    sed -i 's/"-Wno-unused-but-set-variable",//g' build/config/compiler/BUILD.gn

    src/flutter/tools/gn \
        --target-os $BUILD_OS \
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
    ninja -C src/out/${BUILD_OS}_${BUILD_MODE}_${BUILD_ARCH} ${BUILD_TARGET}
fi
