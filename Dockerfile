# =========================
# Stage 1: Builder
# =========================
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install vcpkg
WORKDIR /opt
RUN git clone https://github.com/microsoft/vcpkg.git vcpkg && \
    cd vcpkg && \
    ./bootstrap-vcpkg.sh

ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# Copy project files
WORKDIR /app
COPY CMakeLists.txt vcpkg.json ./
COPY proto/ ./proto/
COPY src/ ./src/
COPY third_party/ ./third_party/

# Remove any pre-generated proto files to force regeneration with container's protoc
RUN rm -rf /app/proto/proto_gen

# Configure and build with CMake
# CMake will install vcpkg dependencies and generate proto files
RUN cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --config Release --parallel $(nproc)

# =========================
# Stage 2: Runtime
# =========================
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libstdc++6 \
    libgcc-s1 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy executable
COPY --from=builder /app/build/GuessIOServer ./GuessIOServer

# Copy runtime libraries from vcpkg
RUN mkdir -p /usr/local/lib
COPY --from=builder /app/build/vcpkg_installed/x64-linux/lib/ /usr/local/lib/

RUN ldconfig

ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

RUN chmod +x ./GuessIOServer
RUN echo '{}' > config.json

EXPOSE 9001 50051

CMD ["./GuessIOServer"]