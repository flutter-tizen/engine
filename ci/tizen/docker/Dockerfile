FROM ghcr.io/flutter-tizen/tizen-tools:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update
RUN apt-get install -y git curl pkg-config ca-certificates xz-utils python python3 libncurses5 && \
    apt-get clean

# Install depot tools.
ENV DEPOT_TOOLS_PATH=/usr/share/depot_tools
ENV PATH=$PATH:${DEPOT_TOOLS_PATH}
RUN git clone --depth=1 https://chromium.googlesource.com/chromium/tools/depot_tools.git ${DEPOT_TOOLS_PATH}
