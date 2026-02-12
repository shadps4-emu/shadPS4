# SPDX-FileCopyrightText: 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

FROM archlinux:latest

RUN pacman-key --init && \
    pacman-key --populate archlinux && \
    pacman -Syu --noconfirm

RUN pacman -S --noconfirm \
    base-devel \
    clang \
	clang19 \
    ninja \
    git \
    ca-certificates \
    wget \
    alsa-lib \
    libpulse \
    openal \
    openssl \
    zlib \
    libedit \
    systemd-libs \
    libevdev \
    sdl2 \
    jack \
    sndio \
    libxtst \
    vulkan-headers \
    vulkan-validation-layers \
    libpng \
    clang-tools-extra \
    cmake \
    libx11 \
    libxrandr \
    libxcursor \
    libxi \
    libxinerama \
    libxss \
    && pacman -Scc --noconfirm
	
RUN ln -sf /usr/lib/llvm19/bin/clang-format /usr/bin/clang-format-19

WORKDIR /workspaces/shadPS4