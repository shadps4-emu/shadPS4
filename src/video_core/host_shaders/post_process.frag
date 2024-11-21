// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#version 450

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 color;

layout (binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform settings {
    float gamma;
} pp;

void main()
{
    vec4 color_linear = texture(texSampler, uv);
    color = pow(color_linear, vec4(1.0/(2.2 + 1.0 - pp.gamma)));
}
