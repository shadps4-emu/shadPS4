# SPDX-FileCopyrightText: 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    git \
    ca-certificates \
    wget \
    libasound2-dev \
    libpulse-dev \
    libopenal-dev \
    libssl-dev \
    zlib1g-dev \
    libedit-dev \
    libudev-dev \
    libevdev-dev \
    libsdl2-dev \
    libjack-dev \
    libsndio-dev \
    libxtst-dev \
    libvulkan-dev \
    vulkan-validationlayers \
    libpng-dev \
    clang-tidy \
    && rm -rf /var/lib/apt/lists/*

RUN wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor -o /usr/share/keyrings/kitware-archive-keyring.gpg \
    && echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ noble main" > /etc/apt/sources.list.d/kitware.list \
    && apt-get update \
    && apt-get install -y cmake \
    && rm -rf /var/lib/apt/lists/*/*

WORKDIR /workspaces/shadPS4