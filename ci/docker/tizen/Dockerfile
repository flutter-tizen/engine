FROM ghcr.io/flutter-tizen/tizen-tools:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y git curl pkg-config ca-certificates xz-utils python python3 && \
    apt-get install -y libncurses5 libfreetype6-dev && \
    apt-get install -y build-essential check meson ninja-build && \
    apt-get install -y libssl-dev libsystemd-dev libjpeg-dev libglib2.0-dev libgstreamer1.0-dev \
                       liblua5.2-dev libfreetype6-dev libfontconfig1-dev libfribidi-dev libavahi-client-dev \
                       libharfbuzz-dev libibus-1.0-dev libx11-dev libxext-dev libxrender-dev libgl1-mesa-dev \
                       libgif-dev libtiff5-dev libpoppler-dev libpoppler-cpp-dev libspectre-dev libraw-dev \
                       librsvg2-dev libudev-dev libmount-dev libdbus-1-dev libpulse-dev libsndfile1-dev \
                       libxcursor-dev libxcomposite-dev libxinerama-dev libxrandr-dev libxtst-dev libxss-dev \
                       libgstreamer-plugins-base1.0-dev doxygen libopenjp2-7-dev libscim-dev libxdamage-dev \
                       libwebp-dev libunwind-dev libheif-dev libinput-dev libluajit-5.1-dev && \
    apt-get install -y xvfb && \
    apt-get clean

# Install depot tools.
ENV DEPOT_TOOLS_PATH=/usr/share/depot_tools
ENV PATH=$PATH:${DEPOT_TOOLS_PATH}
RUN git clone --depth=1 https://chromium.googlesource.com/chromium/tools/depot_tools.git ${DEPOT_TOOLS_PATH}

# Add engine building tools.
ADD tools/* /engine/tools/
ENV PATH=$PATH:/engine/tools

# Build and install EFL.
RUN git clone --depth 1 https://git.enlightenment.org/core/efl.git -b -v1.25.1 /tmp/efl && \
    meson /tmp/efl /tmp/efl/build && \
    ninja -C /tmp/efl/build && \
    ninja -C /tmp/efl/build install && \
    rm -fr /tmp/efl
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/x86_64-linux-gnu/

# Start dbus service when running this container.
ENTRYPOINT /etc/init.d/dbus start && /bin/bash
