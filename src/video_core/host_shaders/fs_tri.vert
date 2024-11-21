// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#version 450

layout(location = 0) out vec2 uv;

void main() {
    vec2 pos = vec2(
        float((gl_VertexIndex & 1u) << 2u),
        float((gl_VertexIndex & 2u) << 1u)
    );
    gl_Position = vec4(pos - vec2(1.0, 1.0), 0.0, 1.0);
    uv = pos * 0.5;
}
