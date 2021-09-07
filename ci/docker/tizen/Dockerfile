FROM ghcr.io/flutter-tizen/tizen-tools:latest

RUN apt-get update && \
    apt-get install -y git curl ca-certificates python python3 xz-utils pkg-config \
                       libncurses5 libfreetype6-dev && \
    apt-get clean

# Install depot tools.
ENV DEPOT_TOOLS_PATH=/usr/share/depot_tools
ENV PATH=$PATH:${DEPOT_TOOLS_PATH}
RUN git clone --depth=1 https://chromium.googlesource.com/chromium/tools/depot_tools.git ${DEPOT_TOOLS_PATH}

# Add engine building tools.
ADD tools/* /engine/tools/
ENV PATH=$PATH:/engine/tools
