#=============================================================================
# Dockerfile — yuleASR Cross-Compilation Environment
#
# Build container with: docker build -t yuleasr-cross -f docker/Dockerfile .
# Run: docker compose -f docker/docker-compose.yml up
#=============================================================================

FROM ubuntu:24.04

LABEL description="yuleASR AUTOSAR BSW Cross-Compilation Environment (ARM Cortex-M7)"
LABEL maintainer="YuleTech"

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

# Install system dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    # Build tools
    build-essential \
    cmake \
    make \
    git \
    wget \
    curl \
    ca-certificates \
    # ARM cross-compiler (GCC)
    gcc-arm-none-eabi \
    # Python (for automation scripts)
    python3 \
    python3-pip \
    # QEMU for simulation
    qemu-system-arm \
    # Debug tools
    gdb-multiarch \
    openocd \
    # Utilities
    file \
    xxd \
    vim-common \
    && rm -rf /var/lib/apt/lists/*

# Install additional Python packages
RUN pip3 install --no-cache-dir pyyaml jinja2

# Install SEGGER JLink (latest stable)
ARG JLINK_VERSION=V796
RUN wget -q https://www.segger.com/downloads/jlink/JLink_Linux_${JLINK_VERSION}_x86_64.tgz -O /tmp/jlink.tgz && \
    mkdir -p /opt/SEGGER/JLink && \
    tar -xzf /tmp/jlink.tgz -C /opt/SEGGER/JLink --strip-components=1 && \
    ln -s /opt/SEGGER/JLink/JLinkExe /usr/local/bin/JLinkExe && \
    ln -s /opt/SEGGER/JLink/JLinkGDBServer /usr/local/bin/JLinkGDBServer && \
    rm /tmp/jlink.tgz

# Create workspace
WORKDIR /workspace

# Default entrypoint
CMD ["/bin/bash"]
