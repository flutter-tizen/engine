#
# Stage for build-engine-base
#
FROM ghcr.io/flutter-tizen/tizen-tools:latest AS build-engine-base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update
RUN apt-get install -y git curl pkg-config ca-certificates xz-utils python python3 libncurses5 && \
    apt-get clean

# Install depot tools.
ENV DEPOT_TOOLS_PATH=/usr/share/depot_tools
ENV PATH=$PATH:${DEPOT_TOOLS_PATH}
RUN git clone --depth=1 https://chromium.googlesource.com/chromium/tools/depot_tools.git ${DEPOT_TOOLS_PATH}


#
# Stage for build-engine-with-efl
#
FROM build-engine-base AS build-engine-with-efl

# Install dependencies for building EFL.
RUN apt-get install -y build-essential check meson ninja-build && \
    apt-get clean
RUN apt-get install -y libssl-dev libsystemd-dev libglib2.0-dev libudev-dev libmount-dev libdbus-1-dev libunwind-dev && \
    apt-get clean
RUN apt-get install -y libjpeg-dev libopenjp2-7-dev libgif-dev libtiff5-dev librsvg2-dev libheif-dev libwebp-dev libraw-dev \
                       libpoppler-dev libpoppler-cpp-dev libspectre-dev libfreetype6-dev libfontconfig1-dev libharfbuzz-dev \
                       libpulse-dev libsndfile1-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
                       libibus-1.0-dev libscim-dev libfribidi-dev libinput-dev liblua5.2-dev libluajit-5.1-dev \
                       libx11-dev libxext-dev libxrender-dev libxcursor-dev libxcomposite-dev libxinerama-dev libxrandr-dev \
                       libxtst-dev libxss-dev libxdamage-dev libgl1-mesa-dev xvfb && \
    apt-get clean

# Build and install EFL for host build.
RUN git clone --depth 1 https://git.enlightenment.org/core/efl.git -b efl-1.25 /tmp/efl && \
    meson -Dbuild-examples=false -Dbuild-tests=false /tmp/efl /tmp/efl/build && \
    ninja -C /tmp/efl/build && \
    ninja -C /tmp/efl/build install && \
    rm -fr /tmp/efl
RUN ldconfig

# Start dbus service when running this container.
ENTRYPOINT /etc/init.d/dbus start && /bin/bash
