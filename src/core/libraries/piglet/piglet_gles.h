// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "common/types.h"

namespace Libraries::Piglet {

// Object names are reserved by glGen* but only become objects once they are bound.
template <typename T>
struct NameTable {
    struct Entry {
        T object{};
        bool created = false;
    };

    std::map<u32, Entry> entries;
    u32 next = 1;

    u32 Gen() {
        const u32 name = next++;
        entries.emplace(name, Entry{});
        return name;
    }

    // Binding a name creates the object on first use.
    T& Bind(u32 name) {
        Entry& entry = entries[name];
        entry.created = true;
        next = std::max(next, name + 1);
        return entry.object;
    }

    T* Find(u32 name) {
        const auto it = entries.find(name);
        return it != entries.end() && it->second.created ? &it->second.object : nullptr;
    }

    // Finds an object that was generated but may not have been bound yet.
    T* FindGenerated(u32 name) {
        const auto it = entries.find(name);
        return it != entries.end() ? &it->second.object : nullptr;
    }

    bool IsObject(u32 name) const {
        const auto it = entries.find(name);
        return it != entries.end() && it->second.created;
    }

    void Delete(u32 name) {
        entries.erase(name);
    }
};

constexpr u32 MAX_VERTEX_ATTRIBS = 16;
constexpr u32 MAX_TEXTURE_UNITS = 32;
constexpr u32 MAX_COLOR_ATTACHMENTS = 4;
constexpr u32 MAX_UNIFORM_BUFFER_BINDINGS = 24;
constexpr u32 MAX_TRANSFORM_FEEDBACK_ATTRIBS = 4;

struct GlBuffer {
    std::vector<u8> data;
    u32 usage = 0x88E4; // GL_STATIC_DRAW
    u32 size = 0;
    bool mapped = false;
    s64 map_offset = 0;
    s64 map_length = 0;
    u32 map_access = 0;
};

struct GlTextureLevel {
    s32 width = 0;
    s32 height = 0;
    s32 depth = 0;
    u32 internal_format = 0;
    bool compressed = false;
};

struct GlTexture {
    u32 target = 0;
    std::map<u32, s32> int_params;
    std::map<u32, float> float_params;
    // Levels are keyed by (face or layer target, level).
    std::map<std::pair<u32, s32>, GlTextureLevel> levels;
    bool immutable = false;
    bool is_canvas = false;
    u32 canvas_handle = 0;
};

struct GlRenderbuffer {
    u32 internal_format = 0x8058; // GL_RGBA8
    s32 width = 0;
    s32 height = 0;
    s32 samples = 0;
};

struct GlFramebufferAttachment {
    u32 type = 0; // 0 (none), GL_TEXTURE or GL_RENDERBUFFER
    u32 name = 0;
    s32 level = 0;
    s32 layer = 0;
    u32 texture_target = 0;
};

struct GlFramebuffer {
    std::map<u32, GlFramebufferAttachment> attachments;
    u32 read_buffer = 0x8CE0; // GL_COLOR_ATTACHMENT0
    std::vector<u32> draw_buffers = {0x8CE0};
};

struct GlShader {
    u32 type = 0;
    std::string source;
    std::vector<u8> binary;
    std::string info_log;
    bool compiled = false;
    bool delete_pending = false;
};

struct GlUniformValue {
    std::array<float, 16> floats{};
    std::array<s32, 16> ints{};
    bool is_float = true;
    u32 count = 0;
};

struct GlProgram {
    std::vector<u32> shaders;
    std::string info_log;
    bool linked = false;
    bool validated = false;
    bool delete_pending = false;
    bool binary_retrievable_hint = false;
    std::map<std::string, u32> attrib_bindings;
    std::map<s32, GlUniformValue> uniforms;
    std::vector<std::string> transform_feedback_varyings;
    u32 transform_feedback_buffer_mode = 0x8C8C; // GL_INTERLEAVED_ATTRIBS
};

struct GlVertexAttrib {
    bool enabled = false;
    s32 size = 4;
    u32 type = 0x1406; // GL_FLOAT
    bool normalized = false;
    bool integer = false;
    s32 stride = 0;
    const void* pointer = nullptr;
    u32 buffer = 0;
    u32 divisor = 0;
    std::array<float, 4> current_float = {0.0f, 0.0f, 0.0f, 1.0f};
    std::array<s32, 4> current_int = {0, 0, 0, 1};
    bool current_is_int = false;
};

struct GlVertexArray {
    u32 element_array_buffer = 0;
    std::array<GlVertexAttrib, MAX_VERTEX_ATTRIBS> attribs;
};

struct GlSampler {
    std::map<u32, s32> int_params;
    std::map<u32, float> float_params;
};

struct GlQuery {
    u32 target = 0;
    bool active = false;
};

struct GlTransformFeedback {
    bool active = false;
    bool paused = false;
};

struct GlIndexedBinding {
    u32 buffer = 0;
    s64 offset = 0;
    s64 size = 0;
};

struct GlSync {
    u32 condition = 0x9117; // GL_SYNC_GPU_COMMANDS_COMPLETE
};

// State of one OpenGL ES context. Piglet has no rendering backend in the emulator yet, so this
// only tracks what the application can observe without a GPU: object names, bindings, simple
// state and the error flag.
struct GlContext {
    explicit GlContext(s32 major_version) : version(major_version) {}

    s32 version;
    u32 error = 0;

    std::set<u32> enabled_caps = {0x0BD0}; // GL_DITHER is enabled by default.
    std::array<s32, 4> viewport{};
    std::array<s32, 4> scissor{};
    bool viewport_initialized = false;
    std::array<float, 4> clear_color{};
    float clear_depth = 1.0f;
    s32 clear_stencil = 0;
    std::array<bool, 4> color_mask = {true, true, true, true};
    bool depth_mask = true;
    u32 depth_func = 0x0201; // GL_LESS
    std::array<float, 2> depth_range = {0.0f, 1.0f};
    std::array<float, 4> blend_color{};
    std::array<u32, 2> blend_equation = {0x8006, 0x8006}; // GL_FUNC_ADD
    std::array<u32, 4> blend_func = {1, 0, 1, 0};         // GL_ONE, GL_ZERO
    u32 cull_face_mode = 0x0405;                          // GL_BACK
    u32 front_face = 0x0901;                              // GL_CCW
    float line_width = 1.0f;
    std::array<float, 2> polygon_offset = {0.0f, 0.0f};
    float sample_coverage_value = 1.0f;
    bool sample_coverage_invert = false;
    u32 active_texture = 0x84C0;                                 // GL_TEXTURE0
    std::map<u32, s32> pixel_store = {{0x0D05, 4}, {0x0CF5, 4}}; // PACK/UNPACK_ALIGNMENT
    std::map<u32, u32> hints;
    std::array<u32, 2> stencil_func = {0x0207, 0x0207}; // GL_ALWAYS (front, back)
    std::array<s32, 2> stencil_ref = {0, 0};
    std::array<u32, 2> stencil_value_mask = {0xFFFFFFFF, 0xFFFFFFFF};
    std::array<u32, 2> stencil_write_mask = {0xFFFFFFFF, 0xFFFFFFFF};
    std::array<std::array<u32, 3>, 2> stencil_op = {{{0x1E00, 0x1E00, 0x1E00}, // GL_KEEP
                                                     {0x1E00, 0x1E00, 0x1E00}}};

    u32 array_buffer = 0;
    // ES 3.0 buffer targets that are only tracked as a binding (copy, pixel pack/unpack, ...).
    std::map<u32, u32> other_buffer_bindings;
    std::array<GlIndexedBinding, MAX_UNIFORM_BUFFER_BINDINGS> uniform_buffer_bindings;
    std::array<GlIndexedBinding, MAX_TRANSFORM_FEEDBACK_ATTRIBS> transform_feedback_bindings;
    GlVertexArray default_vertex_array;
    u32 draw_framebuffer = 0;
    u32 read_framebuffer = 0;
    u32 renderbuffer = 0;
    u32 current_program = 0;
    u32 vertex_array = 0;
    // Texture bound to every target of every unit.
    std::array<std::map<u32, u32>, MAX_TEXTURE_UNITS> texture_bindings;
    std::map<u32, u32> active_queries;
    u32 transform_feedback = 0;
    GlTransformFeedback default_transform_feedback;
    std::vector<u32> default_draw_buffers = {0x0405}; // GL_BACK
    u32 default_read_buffer = 0x0405;

    NameTable<GlBuffer> buffers;
    NameTable<GlTexture> textures;
    NameTable<GlRenderbuffer> renderbuffers;
    NameTable<GlFramebuffer> framebuffers;
    NameTable<GlVertexArray> vertex_arrays;
    NameTable<GlSampler> samplers;
    NameTable<GlQuery> queries;
    NameTable<GlTransformFeedback> transform_feedbacks;
    std::map<u32, GlShader> shaders;
    std::map<u32, GlProgram> programs;
    u32 next_shader_program_name = 1;
    std::map<u64, GlSync> syncs;
    u64 next_sync = 1;
};

// Current context, as set by eglMakeCurrent.
GlContext* GetCurrentGlContext();

} // namespace Libraries::Piglet
