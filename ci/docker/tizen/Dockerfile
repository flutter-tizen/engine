#################
# Builder stage #
#################

FROM debian:buster-slim AS builder

ARG GIT_BRANCH

RUN apt-get update && \
    apt-get install -y git curl ca-certificates python && \
    apt-get clean

# Install depot tools.
ENV DEPOT_TOOLS_PATH=/usr/share/depot_tools
ENV PATH=$PATH:${DEPOT_TOOLS_PATH}
RUN git clone --depth=1 https://chromium.googlesource.com/chromium/tools/depot_tools.git ${DEPOT_TOOLS_PATH}

# Sync engine and dependencies.
WORKDIR /engine
RUN if [ -n "$GIT_BRANCH" ]; then \
        git clone --depth 1 -b $GIT_BRANCH https://github.com/flutter-tizen/engine src/flutter; \
    else \
        git clone --depth 1 https://github.com/flutter-tizen/engine src/flutter; \
    fi

RUN gclient config --name="src/flutter" --deps-file="DEPS" --unmanaged https://github.com/flutter-tizen/engine.git
RUN gclient setdep --var=download_android_deps=False --deps-file=src/flutter/DEPS
RUN gclient sync -f -D --no-history --shallow

# Copy selected .git directories.
RUN mkdir -p cache
RUN for x in $(find src/third_party/ -name .git -a \( -wholename "*/dart/*" -o -wholename "*/skia/*" \)); do \
        mkdir -p cache/$x; \
        cp -fr $x/* cache/$x; \
    done


#############################
# build-engine docker image #
#############################

FROM ghcr.io/flutter-tizen/tizen-tools:latest

RUN apt-get update && \
    apt-get install -y git curl ca-certificates python xz-utils pkg-config \
                       libncurses5 libfreetype6-dev && \
    apt-get clean

COPY --from=builder /usr/share/depot_tools /usr/share/depot_tools
COPY --from=builder /engine/cache /engine/cache
ADD prepare-sync.sh /engine/tools/

ENV PATH=$PATH:/usr/share/depot_tools:/engine/tools
