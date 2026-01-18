#!/bin/bash
# Install Ubuntu/WSL2 dependencies for a126cpp project

set -e

echo "Installing dependencies for a126cpp on Ubuntu/WSL2..."

# Update package list
sudo apt update

# Install essential build tools and dependencies
sudo apt install -y \
    build-essential \
    git \
    make \
    autoconf \
    automake \
    libtool \
    pkg-config \
    cmake \
    ninja-build \
    libasound2-dev \
    libpulse-dev \
    libx11-dev \
    libxext-dev \
    libxrandr-dev \
    libxcursor-dev \
    libxfixes-dev \
    libxi-dev \
    libxss-dev \
    libwayland-dev \
    libxkbcommon-dev \
    libdrm-dev \
    libgbm-dev \
    libgl1-mesa-dev \
    libgles2-mesa-dev \
    libegl1-mesa-dev \
    libdbus-1-dev \
    libudev-dev

# Ubuntu 22.04+ optional dependencies
if [ -f /etc/os-release ]; then
    . /etc/os-release
    if [ "$VERSION_ID" == "22.04" ] || [ "$VERSION_ID" == "23.04" ] || [ "$VERSION_ID" == "23.10" ] || [ "$VERSION_ID" == "24.04" ]; then
        echo "Detected Ubuntu 22.04+, installing additional packages..."
        sudo apt install -y libpipewire-0.3-dev libdecor-0-dev
    fi
fi

echo "Dependencies installed successfully!"
echo ""
echo "You can now build the project:"
echo "  mkdir -p build && cd build"
echo "  cmake .."
echo "  cmake --build ."
