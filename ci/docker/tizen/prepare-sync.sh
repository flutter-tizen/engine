#!/bin/bash -e

DEPS=src/flutter/DEPS

if [ ! -f $DEPS ]; then
    echo "Could not find DEPS file."
    exit 1
fi

cat > .gclient << EOF
solutions = [
  { "name"        : 'src/flutter',
    "url"         : 'https://github.com/flutter-tizen/engine.git',
    "deps_file"   : 'DEPS',
    "managed"     : False,
  },
]
EOF

gclient setdep --var=download_android_deps=False --deps-file=$DEPS

sed -i -e '/src\/ios_tools/,+2d' $DEPS
sed -i -e '/src\/third_party\/vulkan/,+2d' $DEPS
sed -i -e '/src\/third_party\/angle/,+2d' $DEPS
sed -i -e '/src\/third_party\/abseil-cpp/,+2d' $DEPS
sed -i -e '/src\/fuchsia\/sdk\/linux/,+9d' $DEPS

if [ -d /engine/cache ]; then
    cp -fr /engine/cache/* .
fi
