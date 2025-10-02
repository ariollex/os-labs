FROM gcc:14.3.0

RUN apt-get update && apt-get install -y \
    cmake clangd \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
