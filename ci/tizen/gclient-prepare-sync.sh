#!/usr/bin/env bash
# Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# GCLIENT-PREPARE-SYNC #
#
# The `gclient sync` command takes a lot of time because it downloads overly
# many objects to perform builds. To reduce the sync time, this script provides
# following two methods:
#
# 1. Removing unnecesary dependencies.
#    The DEPS describes unnecessary repositories to build the flutter-tizen,
#    such as android, ios, fuchsia related things. By removing thease repositories
#    not to download, the sync time can be shortened.
#
# 2. Shallow clone repositories.
#    Even the `--no-history` option is used, the `gclient sync` fetches all objects
#    of each repository. In CI build situations, these objects are wasted time and
#    storage space. Before run `gclient sync`, these repositories can be shallow
#    fetched by using `fetch --depth 1` to avoid these wastes.
#

set -e

DEPS=src/flutter/DEPS

while [ $# -ne 0 ]; do
    name=$1
    case "$name" in
    --reduce-deps)
        FLAG_REDUCE_DEPS=true
        ;;
    --shallow-sync)
        FLAG_SHALLOW_SYNC=true
        ;;
    --deps)
        shift
        DEPS=$1
        ;;
    -h | --help)
        script_name="$(basename "$0")"
        echo "Usage: $script_name [Options]"
        echo "       $script_name -h|-?|--help"
        echo ""
        echo "Options:"
        echo "  --deps [DEPS]       Set DEPS file path. Default is \`$DEPS\`."
        echo "  --reduce-deps       Remove unnecessary dependencies from DEPS."
        echo "  --shallow-sync      Checkout git repositories with a single depth."
        exit 0
        ;;
    *)
        echo "Unknown argument \`$name\`"
        exit 1
        ;;
    esac

    shift
done

if [ ! -f $DEPS ]; then
    echo "Could not find DEPS file. \`$DEPS\`"
    exit 1
fi

cat >.gclient <<EOF
solutions = [
  { "name"        : 'src/flutter',
    "url"         : 'https://github.com/flutter-tizen/engine.git',
    "deps_file"   : 'DEPS',
    "managed"     : False,
  },
]
EOF

if [[ "$FLAG_REDUCE_DEPS" == "true" ]]; then
    gclient setdep --var=download_android_deps=False --deps-file=$DEPS
    sed -i -e '/src\/ios_tools/,+2d' $DEPS
    sed -i -e '/src\/third_party\/vulkan/,+2d' $DEPS
    sed -i -e '/src\/third_party\/angle/,+2d' $DEPS
    sed -i -e '/src\/fuchsia\/sdk\/linux/,+9d' $DEPS
fi

if [[ "$FLAG_SHALLOW_SYNC" == "true" ]]; then
    export PYTHONPATH=$PYTHONPATH:/usr/share/depot_tools
    python3 "$(dirname -- "$0")/gclient-shallow-sync.py" "$DEPS"
fi
