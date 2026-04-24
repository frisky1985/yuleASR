# ETH-DDS Integration Docker Image
# Version: 2.0.0

# Build stage
FROM ubuntu:22.04 AS builder

# Build arguments
ARG ETH_DDS_VERSION=2.0.0
ARG BUILD_TYPE=Release

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libpthread-stubs0-dev \
    libssl-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . /build/

# Build ETH-DDS
RUN mkdir -p build && cd build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_TESTS=OFF \
    -DENABLE_ETHERNET=ON \
    -DENABLE_DDS=ON \
    -DENABLE_TSN=ON \
    -DENABLE_AUTOSAR_CLASSIC=ON \
    -DENABLE_AUTOSAR_ADAPTIVE=ON \
    -DENABLE_DDS_SECURITY=ON \
    -DENABLE_DDS_ADVANCED=ON && \
    make -j$(nproc) && \
    make install

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

# Copy built artifacts from builder
COPY --from=builder /usr/include/eth_dds /usr/include/eth_dds
COPY --from=builder /usr/lib/libeth_dds* /usr/lib/
COPY --from=builder /usr/lib/cmake/eth_dds /usr/lib/cmake/eth_dds
COPY --from=builder /usr/bin/eth_dds* /usr/bin/

# Update library cache
RUN ldconfig

# Create non-root user
RUN useradd -m -s /bin/bash eth-dds

# Set working directory
WORKDIR /app

# Switch to non-root user
USER eth-dds

# Environment variables
ENV ETH_DDS_VERSION=2.0.0
ENV LD_LIBRARY_PATH=/usr/lib:${LD_LIBRARY_PATH}

# Labels
LABEL maintainer="ETH-DDS Team <support@eth-dds.org>" \
      version="${ETH_DDS_VERSION}" \
      description="ETH-DDS Integration Library" \
      org.opencontainers.image.title="ETH-DDS Integration" \
      org.opencontainers.image.description="High-performance data communication framework" \
      org.opencontainers.image.version="${ETH_DDS_VERSION}" \
      org.opencontainers.image.licenses="Apache-2.0"

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD echo "ETH-DDS version: ${ETH_DDS_VERSION}" || exit 1

# Default command
CMD ["echo", "ETH-DDS Integration Library v${ETH_DDS_VERSION}"]
