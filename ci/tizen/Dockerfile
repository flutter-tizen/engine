###############################
### Stage for building LLVM ###
###############################

FROM ubuntu:22.04 AS llvm

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y git zip build-essential cmake ninja-build clang-11 && \
    apt-get clean

COPY build_llvm.sh .

RUN /build_llvm.sh


##############################
### Create a release image ###
##############################

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y binutils-arm-linux-gnueabi binutils-aarch64-linux-gnu binutils-i686-linux-gnu && \
    apt-get install -y git curl pkg-config ca-certificates python3 python3-pip rpm2cpio cpio && \
    apt-get clean

# Copy build artifacts from the previous stage.
COPY --from=llvm /toolchains/ /tizen_tools/toolchains/
