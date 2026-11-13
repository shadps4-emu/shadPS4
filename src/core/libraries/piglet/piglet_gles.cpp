// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string>
#include <type_traits>
#include <utility>

#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/piglet/piglet_gles.h"
#include "core/libraries/piglet/piglet_internal.h"

namespace Libraries::Piglet {

namespace {

// OpenGL ES enumerants used below.
constexpr u32 GL_NO_ERROR = 0;
constexpr u32 GL_INVALID_ENUM = 0x0500;
constexpr u32 GL_INVALID_VALUE = 0x0501;
constexpr u32 GL_INVALID_OPERATION = 0x0502;
constexpr u32 GL_OUT_OF_MEMORY = 0x0505;
constexpr u32 GL_ARRAY_BUFFER = 0x8892;
constexpr u32 GL_ELEMENT_ARRAY_BUFFER = 0x8893;
constexpr u32 GL_TEXTURE_2D = 0x0DE1;
constexpr u32 GL_TEXTURE0 = 0x84C0;
constexpr u32 GL_FRAMEBUFFER = 0x8D40;
constexpr u32 GL_RENDERBUFFER = 0x8D41;
constexpr u32 GL_READ_FRAMEBUFFER = 0x8CA8;
constexpr u32 GL_DRAW_FRAMEBUFFER = 0x8CA9;
constexpr u32 GL_VERTEX_SHADER = 0x8B31;
constexpr u32 GL_FRAGMENT_SHADER = 0x8B30;
// Largest buffer that is stored. Piglet's video memory heap is far smaller than this.
constexpr s64 MAX_BUFFER_SIZE = 512 * 1024 * 1024;

constexpr u32 GL_FRONT = 0x0404;
constexpr u32 GL_BACK = 0x0405;
constexpr u32 GL_FRONT_AND_BACK = 0x0408;

// Compressed texture formats reported by GL_COMPRESSED_TEXTURE_FORMATS.
constexpr std::array<u32, 17> COMPRESSED_FORMATS = {
    33776, 33777, 33778, 33779, 36196, 36492, 36493, 36494, 36495,
    35952, 35953, 35954, 35955, 36283, 36284, 36285, 36286,
};

// Strings returned by glGetString.
constexpr const char* VENDOR_STRING = "Sony Interactive Entertainment Inc.";
constexpr const char* RENDERER_STRING = "Piglet";
constexpr const char* VERSION_STRING = "OpenGL ES 2.0 Piglet";
constexpr const char* SHADING_LANGUAGE_VERSION_STRING = "OpenGL ES GLSL ES 2.0";

// Extensions, in the order Piglet registers them.
constexpr std::array EXTENSIONS = {
    "GL_OES_texture_npot",
    "GL_EXT_texture_filter_anisotropic",
    "GL_EXT_texture_rg",
    "GL_OES_rgb8_rgba8",
    "GL_OES_depth_texture",
    "GL_OES_depth32",
    "GL_OES_fbo_render_mipmap",
    "GL_EXT_texture_format_BGRA8888",
    "GL_EXT_read_format_bgra",
    "GL_OES_texture_float",
    "GL_OES_texture_half_float",
    "GL_EXT_color_buffer_half_float",
    "GL_OES_texture_float_linear",
    "GL_OES_texture_half_float_linear",
    "GL_OES_vertex_half_float",
    "GL_OES_element_index_uint",
    "GL_EXT_texture_compression_dxt1",
    "GL_EXT_texture_compression_dxt3",
    "GL_EXT_texture_compression_dxt5",
    "GL_EXT_texture_compression_s3tc",
    "GL_EXT_texture_storage",
    "GL_OES_compressed_ETC1_RGB8_texture",
    "GL_OES_standard_derivatives",
    "GL_EXT_shader_texture_lod",
    "GL_ARB_texture_compression_bptc",
    "GL_EXT_texture_compression_latc",
    "GL_EXT_texture_compression_rgtc",
    "GL_OES_texture_stencil8",
    "GL_EXT_color_buffer_float",
    "GL_EXT_debug_marker",
    "GL_EXT_draw_instanced",
    "GL_EXT_instanced_arrays",
    "GL_SCE_piglet_shader_binary",
    "GL_SCE_orbis_canvas_texture",
    "GL_SCE_texture_resource",
    "GL_SCE_commandbuffer_inject",
    "GL_EXT_disjoint_timer_query",
};

// Piglet has no shader compiler on retail hardware: shaders have to be supplied as precompiled
// binaries through glShaderBinary.
constexpr const char* SHADER_COMPILER_UNSUPPORTED = "Shader compiler not supported!\n";

// Returns the current context, or logs and returns null when there is none.
GlContext* RequireContext() {
    GlContext* gl = GetCurrentGlContext();
    if (gl == nullptr) {
        LOG_ERROR(Lib_Piglet, "No context!");
    }
    return gl;
}

// Only the first error is kept until it is read.
void SetGlError(GlContext& gl, u32 error) {
    if (gl.error == GL_NO_ERROR) {
        gl.error = error;
    }
}

bool IsFace(u32 face) {
    return face == GL_FRONT || face == GL_BACK || face == GL_FRONT_AND_BACK;
}

bool IsCompareFunc(u32 func) {
    return func >= 0x0200 && func <= 0x0207;
}

bool IsStencilOp(u32 op) {
    return op == 0 || op == 0x1E00 || op == 0x1E01 || op == 0x1E02 || op == 0x1E03 ||
           op == 0x150A || op == 0x8507 || op == 0x8508;
}

bool IsBlendEquation(u32 mode) {
    return mode == 0x8006 || mode == 0x800A || mode == 0x800B;
}

bool IsBlendFactor(u32 factor) {
    return factor == 0 || factor == 1 || (factor >= 0x0300 && factor <= 0x0308) ||
           (factor >= 0x8001 && factor <= 0x8004);
}

bool IsCapability(u32 cap) {
    switch (cap) {
    case 0x0BE2: // GL_BLEND
    case 0x0B44: // GL_CULL_FACE
    case 0x0B71: // GL_DEPTH_TEST
    case 0x0BD0: // GL_DITHER
    case 0x8037: // GL_POLYGON_OFFSET_FILL
    case 0x809E: // GL_SAMPLE_ALPHA_TO_COVERAGE
    case 0x80A0: // GL_SAMPLE_COVERAGE
    case 0x0C11: // GL_SCISSOR_TEST
    case 0x0B90: // GL_STENCIL_TEST
        return true;
    default:
        return false;
    }
}

// Stencil state is stored as [front, back].
template <typename Fn>
void ForEachFace(u32 face, Fn&& fn) {
    if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
        fn(0);
    }
    if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
        fn(1);
    }
}

template <typename Table>
void GenNames(GlContext& gl, Table& table, s32 n, u32* names) {
    if (n < 0) {
        SetGlError(gl, GL_INVALID_VALUE);
        return;
    }
    for (s32 i = 0; i < n; i++) {
        names[i] = table.Gen();
    }
}

// Deletes the named objects. on_delete runs for every object that existed, so bindings to it can
// be reset.
template <typename Table, typename Fn>
void DeleteNames(GlContext& gl, Table& table, s32 n, const u32* names, Fn&& on_delete) {
    if (n < 0) {
        SetGlError(gl, GL_INVALID_VALUE);
        return;
    }
    for (s32 i = 0; i < n; i++) {
        if (names[i] != 0) {
            on_delete(names[i]);
            table.Delete(names[i]);
        }
    }
}

u32& ElementArrayBufferBinding(GlContext& gl) {
    if (gl.vertex_array != 0) {
        if (auto* vao = gl.vertex_arrays.Find(gl.vertex_array)) {
            return vao->element_array_buffer;
        }
    }
    return gl.default_vertex_array.element_array_buffer;
}

u32* BufferBinding(GlContext& gl, u32 target) {
    if (target == GL_ARRAY_BUFFER) {
        return &gl.array_buffer;
    }
    if (target == GL_ELEMENT_ARRAY_BUFFER) {
        return &ElementArrayBufferBinding(gl);
    }
    switch (target) {
    case 0x8F36: // GL_COPY_READ_BUFFER
    case 0x8F37: // GL_COPY_WRITE_BUFFER
    case 0x88EB: // GL_PIXEL_PACK_BUFFER
    case 0x88EC: // GL_PIXEL_UNPACK_BUFFER
    case 0x8C8E: // GL_TRANSFORM_FEEDBACK_BUFFER
    case 0x8A11: // GL_UNIFORM_BUFFER
        return gl.version >= 3 ? &gl.other_buffer_bindings[target] : nullptr;
    default:
        return nullptr;
    }
}

bool IsBufferUsage(u32 usage) {
    return usage == 0x88E0 || usage == 0x88E4 || usage == 0x88E8;
}

// Value of a piece of context state, for the glGet* queries.
struct StateValue {
    std::array<double, 17> v{};
    u32 count = 1;
};

// Implementation limits. The values were read from libScePigletv2VSH's own glGet implementation.
bool QueryLimit(const GlContext& gl, u32 pname, StateValue& out) {
    auto set = [&](std::initializer_list<double> values) {
        out.count = 0;
        for (const double value : values) {
            out.v[out.count++] = value;
        }
        return true;
    };
    switch (pname) {
    case 0x0D33: // GL_MAX_TEXTURE_SIZE
    case 0x84E8: // GL_MAX_RENDERBUFFER_SIZE
        return set({4096});
    case 0x0D3A: // GL_MAX_VIEWPORT_DIMS
        return set({4096, 4096});
    case 0x0D50: // GL_SUBPIXEL_BITS
        return set({4});
    case 0x846D: // GL_ALIASED_POINT_SIZE_RANGE
        return set({1, 16});
    case 0x846E: // GL_ALIASED_LINE_WIDTH_RANGE
        return set({0.125, 128});
    case 0x84FF: // GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
        return set({16});
    case 0x851C: // GL_MAX_CUBE_MAP_TEXTURE_SIZE
        return set({2048});
    case 0x86A2: // GL_NUM_COMPRESSED_TEXTURE_FORMATS
        return set({static_cast<double>(COMPRESSED_FORMATS.size())});
    case 0x86A3: // GL_COMPRESSED_TEXTURE_FORMATS
        out.count = 0;
        for (const u32 format : COMPRESSED_FORMATS) {
            out.v[out.count++] = format;
        }
        return true;
    case 0x8869: // GL_MAX_VERTEX_ATTRIBS
    case 0x8872: // GL_MAX_TEXTURE_IMAGE_UNITS
    case 0x8B4C: // GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS
        return set({16});
    case 0x8B4D: // GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
        return set({32});
    case 0x8DF8: // GL_SHADER_BINARY_FORMATS
        // Piglet only reports a single entry here, which is zero.
        return set({0});
    case 0x8DF9: // GL_NUM_SHADER_BINARY_FORMATS
        return set({2});
    case 0x8DFA: // GL_SHADER_COMPILER
        // There is no shader compiler, see glCompileShader.
        return set({0});
    case 0x8DFB: // GL_MAX_VERTEX_UNIFORM_VECTORS
        return set({256});
    case 0x8DFC: // GL_MAX_VARYING_VECTORS
        return set({15});
    case 0x8DFD: // GL_MAX_FRAGMENT_UNIFORM_VECTORS
        return set({224});
    default:
        break;
    }
    if (gl.version < 3) {
        return false;
    }
    switch (pname) {
    case 0x8073: // GL_MAX_3D_TEXTURE_SIZE
    case 0x88FF: // GL_MAX_ARRAY_TEXTURE_LAYERS
        return set({256});
    case 0x80E8: // GL_MAX_ELEMENTS_VERTICES
    case 0x80E9: // GL_MAX_ELEMENTS_INDICES
        return set({1073741824});
    case 0x821B: // GL_MAJOR_VERSION
        return set({3});
    case 0x84FD: // GL_MAX_TEXTURE_LOD_BIAS
        return set({2});
    case 0x8824: // GL_MAX_DRAW_BUFFERS
    case 0x8CDF: // GL_MAX_COLOR_ATTACHMENTS
    case 0x8C80: // GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS
    case 0x8C8B: // GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS
    case 0x8A34: // GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT
        return set({4});
    case 0x8904: // GL_MIN_PROGRAM_TEXEL_OFFSET
        return set({-8});
    case 0x8905: // GL_MAX_PROGRAM_TEXEL_OFFSET
        return set({7});
    case 0x8A2B: // GL_MAX_VERTEX_UNIFORM_BLOCKS
    case 0x8A2D: // GL_MAX_FRAGMENT_UNIFORM_BLOCKS
        return set({12});
    case 0x8A2E: // GL_MAX_COMBINED_UNIFORM_BLOCKS
    case 0x8A2F: // GL_MAX_UNIFORM_BUFFER_BINDINGS
        return set({24});
    case 0x8A30: // GL_MAX_UNIFORM_BLOCK_SIZE
        return set({16384});
    case 0x8A31: // GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS
    case 0x8B49: // GL_MAX_FRAGMENT_UNIFORM_COMPONENTS
        return set({896});
    case 0x8B4A: // GL_MAX_VERTEX_UNIFORM_COMPONENTS
        return set({1024});
    case 0x8B4B: // GL_MAX_VARYING_COMPONENTS
    case 0x9125: // GL_MAX_FRAGMENT_INPUT_COMPONENTS
        return set({60});
    case 0x8C8A: // GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS
    case 0x9122: // GL_MAX_VERTEX_OUTPUT_COMPONENTS
        return set({64});
    case 0x8D57: // GL_MAX_SAMPLES
        return set({8});
    case 0x8D6B: // GL_MAX_ELEMENT_INDEX
        return set({16777215});
    case 0x9111: // GL_MAX_SERVER_WAIT_TIMEOUT
        return set({1000000000});
    default:
        return false;
    }
}

bool QueryState(const GlContext& gl, u32 pname, StateValue& out) {
    auto set = [&](std::initializer_list<double> values) {
        out.count = 0;
        for (const double value : values) {
            out.v[out.count++] = value;
        }
        return true;
    };
    switch (pname) {
    case 0x0BA2: // GL_VIEWPORT
        return set({static_cast<double>(gl.viewport[0]), static_cast<double>(gl.viewport[1]),
                    static_cast<double>(gl.viewport[2]), static_cast<double>(gl.viewport[3])});
    case 0x0C10: // GL_SCISSOR_BOX
        return set({static_cast<double>(gl.scissor[0]), static_cast<double>(gl.scissor[1]),
                    static_cast<double>(gl.scissor[2]), static_cast<double>(gl.scissor[3])});
    case 0x0C22: // GL_COLOR_CLEAR_VALUE
        return set({gl.clear_color[0], gl.clear_color[1], gl.clear_color[2], gl.clear_color[3]});
    case 0x0B73: // GL_DEPTH_CLEAR_VALUE
        return set({gl.clear_depth});
    case 0x0B91: // GL_STENCIL_CLEAR_VALUE
        return set({static_cast<double>(gl.clear_stencil)});
    case 0x0C23: // GL_COLOR_WRITEMASK
        return set({gl.color_mask[0] ? 1.0 : 0.0, gl.color_mask[1] ? 1.0 : 0.0,
                    gl.color_mask[2] ? 1.0 : 0.0, gl.color_mask[3] ? 1.0 : 0.0});
    case 0x0B72: // GL_DEPTH_WRITEMASK
        return set({gl.depth_mask ? 1.0 : 0.0});
    case 0x0B74: // GL_DEPTH_FUNC
        return set({static_cast<double>(gl.depth_func)});
    case 0x0B70: // GL_DEPTH_RANGE
        return set({gl.depth_range[0], gl.depth_range[1]});
    case 0x8005: // GL_BLEND_COLOR
        return set({gl.blend_color[0], gl.blend_color[1], gl.blend_color[2], gl.blend_color[3]});
    case 0x8009: // GL_BLEND_EQUATION_RGB
        return set({static_cast<double>(gl.blend_equation[0])});
    case 0x883D: // GL_BLEND_EQUATION_ALPHA
        return set({static_cast<double>(gl.blend_equation[1])});
    case 0x80C9: // GL_BLEND_SRC_RGB
        return set({static_cast<double>(gl.blend_func[0])});
    case 0x80C8: // GL_BLEND_DST_RGB
        return set({static_cast<double>(gl.blend_func[1])});
    case 0x80CB: // GL_BLEND_SRC_ALPHA
        return set({static_cast<double>(gl.blend_func[2])});
    case 0x80CA: // GL_BLEND_DST_ALPHA
        return set({static_cast<double>(gl.blend_func[3])});
    case 0x0B45: // GL_CULL_FACE_MODE
        return set({static_cast<double>(gl.cull_face_mode)});
    case 0x0B46: // GL_FRONT_FACE
        return set({static_cast<double>(gl.front_face)});
    case 0x0B21: // GL_LINE_WIDTH
        return set({gl.line_width});
    case 0x8038: // GL_POLYGON_OFFSET_FACTOR
        return set({gl.polygon_offset[0]});
    case 0x2A00: // GL_POLYGON_OFFSET_UNITS
        return set({gl.polygon_offset[1]});
    case 0x80AA: // GL_SAMPLE_COVERAGE_VALUE
        return set({gl.sample_coverage_value});
    case 0x80AB: // GL_SAMPLE_COVERAGE_INVERT
        return set({gl.sample_coverage_invert ? 1.0 : 0.0});
    case 0x84E0: // GL_ACTIVE_TEXTURE
        return set({static_cast<double>(gl.active_texture)});
    case 0x8894: // GL_ARRAY_BUFFER_BINDING
        return set({static_cast<double>(gl.array_buffer)});
    case 0x8895: { // GL_ELEMENT_ARRAY_BUFFER_BINDING
        u32 binding = gl.default_vertex_array.element_array_buffer;
        if (gl.vertex_array != 0) {
            const auto it = gl.vertex_arrays.entries.find(gl.vertex_array);
            if (it != gl.vertex_arrays.entries.end()) {
                binding = it->second.object.element_array_buffer;
            }
        }
        return set({static_cast<double>(binding)});
    }
    case 0x8CA6: // GL_DRAW_FRAMEBUFFER_BINDING / GL_FRAMEBUFFER_BINDING
        return set({static_cast<double>(gl.draw_framebuffer)});
    case 0x8CAA: // GL_READ_FRAMEBUFFER_BINDING
        return set({static_cast<double>(gl.read_framebuffer)});
    case 0x8CA7: // GL_RENDERBUFFER_BINDING
        return set({static_cast<double>(gl.renderbuffer)});
    case 0x8B8D: // GL_CURRENT_PROGRAM
        return set({static_cast<double>(gl.current_program)});
    case 0x8069: // GL_TEXTURE_BINDING_2D
    {
        const auto& bindings = gl.texture_bindings[gl.active_texture - GL_TEXTURE0];
        const auto it = bindings.find(GL_TEXTURE_2D);
        return set({static_cast<double>(it != bindings.end() ? it->second : 0)});
    }
    case 0x85B5: // GL_VERTEX_ARRAY_BINDING
        return set({static_cast<double>(gl.vertex_array)});
    case 0x0B92: // GL_STENCIL_FUNC
        return set({static_cast<double>(gl.stencil_func[0])});
    case 0x0B97: // GL_STENCIL_REF
        return set({static_cast<double>(gl.stencil_ref[0])});
    case 0x0B93: // GL_STENCIL_VALUE_MASK
        return set({static_cast<double>(gl.stencil_value_mask[0])});
    case 0x0B98: // GL_STENCIL_WRITEMASK
        return set({static_cast<double>(gl.stencil_write_mask[0])});
    case 0x0B94: // GL_STENCIL_FAIL
        return set({static_cast<double>(gl.stencil_op[0][0])});
    case 0x0B95: // GL_STENCIL_PASS_DEPTH_FAIL
        return set({static_cast<double>(gl.stencil_op[0][1])});
    case 0x0B96: // GL_STENCIL_PASS_DEPTH_PASS
        return set({static_cast<double>(gl.stencil_op[0][2])});
    case 0x8800: // GL_STENCIL_BACK_FUNC
        return set({static_cast<double>(gl.stencil_func[1])});
    case 0x8CA3: // GL_STENCIL_BACK_REF
        return set({static_cast<double>(gl.stencil_ref[1])});
    case 0x8CA4: // GL_STENCIL_BACK_VALUE_MASK
        return set({static_cast<double>(gl.stencil_value_mask[1])});
    case 0x8CA5: // GL_STENCIL_BACK_WRITEMASK
        return set({static_cast<double>(gl.stencil_write_mask[1])});
    case 0x8801: // GL_STENCIL_BACK_FAIL
        return set({static_cast<double>(gl.stencil_op[1][0])});
    case 0x8802: // GL_STENCIL_BACK_PASS_DEPTH_FAIL
        return set({static_cast<double>(gl.stencil_op[1][1])});
    case 0x8803: // GL_STENCIL_BACK_PASS_DEPTH_PASS
        return set({static_cast<double>(gl.stencil_op[1][2])});
    case 0x0D05: // GL_PACK_ALIGNMENT
        return set({static_cast<double>(gl.pixel_store.at(0x0D05))});
    case 0x0CF5: // GL_UNPACK_ALIGNMENT
        return set({static_cast<double>(gl.pixel_store.at(0x0CF5))});
    default:
        return QueryLimit(gl, pname, out);
    }
}

s32 ToInteger(double value) {
    return static_cast<s32>(std::llround(value));
}

template <typename T, typename Convert>
void GetState(u32 pname, T* params, Convert&& convert) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    StateValue value;
    if (!QueryState(*gl, pname, value)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        LOG_ERROR(Lib_Piglet, "Unsupported pname {:#x}", pname);
        return;
    }
    for (u32 i = 0; i < value.count; i++) {
        params[i] = convert(value.v[i]);
    }
}

// Writes a NUL terminated string that fits in buf_size, reporting how much was written.
void CopyString(const std::string& text, s32 buf_size, s32* length, char* out) {
    const s32 count = std::clamp(static_cast<s32>(text.size()), 0, std::max(buf_size - 1, 0));
    if (buf_size > 0 && out != nullptr) {
        std::memcpy(out, text.data(), count);
        out[count] = '\0';
    }
    if (length != nullptr) {
        *length = buf_size > 0 ? count : 0;
    }
}

} // namespace

u32 PS4_SYSV_ABI glGetError() {
    GlContext* gl = GetCurrentGlContext();
    if (gl == nullptr) {
        return GL_NO_ERROR;
    }
    return std::exchange(gl->error, GL_NO_ERROR);
}

const char* PS4_SYSV_ABI glGetString(u32 name) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return nullptr;
    }
    switch (name) {
    case 0x1F00: // GL_VENDOR
        return VENDOR_STRING;
    case 0x1F01: // GL_RENDERER
        return RENDERER_STRING;
    case 0x1F02: // GL_VERSION
        return VERSION_STRING;
    case 0x1F03: { // GL_EXTENSIONS
        static const std::string extensions = [] {
            std::string result;
            for (const char* extension : EXTENSIONS) {
                result += extension;
                result += ' ';
            }
            return result;
        }();
        return extensions.c_str();
    }
    case 0x8B8C: // GL_SHADING_LANGUAGE_VERSION
        return SHADING_LANGUAGE_VERSION_STRING;
    default:
        SetGlError(*gl, GL_INVALID_ENUM);
        LOG_ERROR(Lib_Piglet, "Invalid name {:#x}", name);
        return nullptr;
    }
}

const char* PS4_SYSV_ABI glGetStringi(u32 name, u32 index) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return nullptr;
    }
    if (name != 0x1F03) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return nullptr;
    }
    if (index >= EXTENSIONS.size()) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return nullptr;
    }
    return EXTENSIONS[index];
}

void PS4_SYSV_ABI glGetIntegerv(u32 pname, s32* params) {
    GetState(pname, params, [](double v) { return ToInteger(v); });
}

void PS4_SYSV_ABI glGetInteger64v(u32 pname, s64* params) {
    GetState(pname, params, [](double v) { return static_cast<s64>(std::llround(v)); });
}

void PS4_SYSV_ABI glGetFloatv(u32 pname, float* params) {
    GetState(pname, params, [](double v) { return static_cast<float>(v); });
}

void PS4_SYSV_ABI glGetBooleanv(u32 pname, u8* params) {
    GetState(pname, params, [](double v) { return static_cast<u8>(v != 0.0 ? 1 : 0); });
}

u8 PS4_SYSV_ABI glIsEnabled(u32 cap) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return 0;
    }
    if (!IsCapability(cap)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return 0;
    }
    return gl->enabled_caps.contains(cap) ? 1 : 0;
}

void PS4_SYSV_ABI glEnable(u32 cap) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsCapability(cap)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    gl->enabled_caps.insert(cap);
}

void PS4_SYSV_ABI glDisable(u32 cap) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsCapability(cap)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    gl->enabled_caps.erase(cap);
}

void PS4_SYSV_ABI glFinish() {
    // TODO: Wait for the GPU once there is a rendering backend.
}

void PS4_SYSV_ABI glFlush() {
    // TODO: Submit pending work once there is a rendering backend.
}

void PS4_SYSV_ABI glActiveTexture(u32 texture) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (texture < GL_TEXTURE0 || texture >= GL_TEXTURE0 + MAX_TEXTURE_UNITS) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    gl->active_texture = texture;
}

void PS4_SYSV_ABI glBlendColor(float red, float green, float blue, float alpha) {
    if (GlContext* gl = RequireContext()) {
        gl->blend_color = {std::clamp(red, 0.0f, 1.0f), std::clamp(green, 0.0f, 1.0f),
                           std::clamp(blue, 0.0f, 1.0f), std::clamp(alpha, 0.0f, 1.0f)};
    }
}

void PS4_SYSV_ABI glBlendEquationSeparate(u32 mode_rgb, u32 mode_alpha) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsBlendEquation(mode_rgb) || !IsBlendEquation(mode_alpha)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    gl->blend_equation = {mode_rgb, mode_alpha};
}

void PS4_SYSV_ABI glBlendEquation(u32 mode) {
    glBlendEquationSeparate(mode, mode);
}

void PS4_SYSV_ABI glBlendFuncSeparate(u32 src_rgb, u32 dst_rgb, u32 src_alpha, u32 dst_alpha) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    for (const u32 factor : {src_rgb, dst_rgb, src_alpha, dst_alpha}) {
        if (!IsBlendFactor(factor)) {
            SetGlError(*gl, GL_INVALID_ENUM);
            return;
        }
    }
    gl->blend_func = {src_rgb, dst_rgb, src_alpha, dst_alpha};
}

void PS4_SYSV_ABI glBlendFunc(u32 sfactor, u32 dfactor) {
    glBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
}

void PS4_SYSV_ABI glClearColor(float red, float green, float blue, float alpha) {
    if (GlContext* gl = RequireContext()) {
        gl->clear_color = {std::clamp(red, 0.0f, 1.0f), std::clamp(green, 0.0f, 1.0f),
                           std::clamp(blue, 0.0f, 1.0f), std::clamp(alpha, 0.0f, 1.0f)};
    }
}

void PS4_SYSV_ABI glClearDepthf(float depth) {
    if (GlContext* gl = RequireContext()) {
        gl->clear_depth = std::clamp(depth, 0.0f, 1.0f);
    }
}

void PS4_SYSV_ABI glClearStencil(s32 s) {
    if (GlContext* gl = RequireContext()) {
        gl->clear_stencil = s;
    }
}

void PS4_SYSV_ABI glColorMask(u8 red, u8 green, u8 blue, u8 alpha) {
    if (GlContext* gl = RequireContext()) {
        gl->color_mask = {red != 0, green != 0, blue != 0, alpha != 0};
    }
}

void PS4_SYSV_ABI glCullFace(u32 mode) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsFace(mode)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    gl->cull_face_mode = mode;
}

void PS4_SYSV_ABI glDepthFunc(u32 func) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsCompareFunc(func)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    gl->depth_func = func;
}

void PS4_SYSV_ABI glDepthMask(u8 flag) {
    if (GlContext* gl = RequireContext()) {
        gl->depth_mask = flag != 0;
    }
}

void PS4_SYSV_ABI glDepthRangef(float n, float f) {
    if (GlContext* gl = RequireContext()) {
        gl->depth_range = {std::clamp(n, 0.0f, 1.0f), std::clamp(f, 0.0f, 1.0f)};
    }
}

void PS4_SYSV_ABI glFrontFace(u32 mode) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (mode != 0x0900 && mode != 0x0901) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    gl->front_face = mode;
}

void PS4_SYSV_ABI glHint(u32 target, u32 mode) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if ((target != 0x8192 && target != 0x8B8B) || mode < 0x1100 || mode > 0x1102) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    gl->hints[target] = mode;
}

void PS4_SYSV_ABI glLineWidth(float width) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!(width > 0.0f)) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    gl->line_width = width;
}

void PS4_SYSV_ABI glPixelStorei(u32 pname, s32 param) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    switch (pname) {
    case 0x0D05: // GL_PACK_ALIGNMENT
    case 0x0CF5: // GL_UNPACK_ALIGNMENT
        if (param != 1 && param != 2 && param != 4 && param != 8) {
            SetGlError(*gl, GL_INVALID_VALUE);
            return;
        }
        break;
    case 0x0CF2: // GL_UNPACK_ROW_LENGTH
    case 0x0CF3: // GL_UNPACK_SKIP_ROWS
    case 0x0CF4: // GL_UNPACK_SKIP_PIXELS
    case 0x806D: // GL_UNPACK_IMAGE_HEIGHT
    case 0x806E: // GL_UNPACK_SKIP_IMAGES
    case 0x0D02: // GL_PACK_ROW_LENGTH
    case 0x0D03: // GL_PACK_SKIP_ROWS
    case 0x0D04: // GL_PACK_SKIP_PIXELS
        if (gl->version < 3) {
            SetGlError(*gl, GL_INVALID_ENUM);
            return;
        }
        if (param < 0) {
            SetGlError(*gl, GL_INVALID_VALUE);
            return;
        }
        break;
    default:
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    gl->pixel_store[pname] = param;
}

void PS4_SYSV_ABI glPolygonOffset(float factor, float units) {
    if (GlContext* gl = RequireContext()) {
        gl->polygon_offset = {factor, units};
    }
}

void PS4_SYSV_ABI glSampleCoverage(float value, u8 invert) {
    if (GlContext* gl = RequireContext()) {
        gl->sample_coverage_value = std::clamp(value, 0.0f, 1.0f);
        gl->sample_coverage_invert = invert != 0;
    }
}

void PS4_SYSV_ABI glScissor(s32 x, s32 y, s32 width, s32 height) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (width < 0 || height < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    gl->scissor = {x, y, width, height};
}

void PS4_SYSV_ABI glViewport(s32 x, s32 y, s32 width, s32 height) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (width < 0 || height < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    gl->viewport = {x, y, width, height};
}

void PS4_SYSV_ABI glStencilFuncSeparate(u32 face, u32 func, s32 ref, u32 mask) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsFace(face) || !IsCompareFunc(func)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    ForEachFace(face, [&](u32 i) {
        gl->stencil_func[i] = func;
        gl->stencil_ref[i] = ref;
        gl->stencil_value_mask[i] = mask;
    });
}

void PS4_SYSV_ABI glStencilFunc(u32 func, s32 ref, u32 mask) {
    glStencilFuncSeparate(GL_FRONT_AND_BACK, func, ref, mask);
}

void PS4_SYSV_ABI glStencilMaskSeparate(u32 face, u32 mask) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsFace(face)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    ForEachFace(face, [&](u32 i) { gl->stencil_write_mask[i] = mask; });
}

void PS4_SYSV_ABI glStencilMask(u32 mask) {
    glStencilMaskSeparate(GL_FRONT_AND_BACK, mask);
}

void PS4_SYSV_ABI glStencilOpSeparate(u32 face, u32 sfail, u32 dpfail, u32 dppass) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsFace(face) || !IsStencilOp(sfail) || !IsStencilOp(dpfail) || !IsStencilOp(dppass)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    ForEachFace(face, [&](u32 i) { gl->stencil_op[i] = {sfail, dpfail, dppass}; });
}

void PS4_SYSV_ABI glStencilOp(u32 sfail, u32 dpfail, u32 dppass) {
    glStencilOpSeparate(GL_FRONT_AND_BACK, sfail, dpfail, dppass);
}

void PS4_SYSV_ABI glGenBuffers(s32 n, u32* buffers) {
    if (GlContext* gl = RequireContext()) {
        GenNames(*gl, gl->buffers, n, buffers);
    }
}

void PS4_SYSV_ABI glDeleteBuffers(s32 n, const u32* buffers) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    DeleteNames(*gl, gl->buffers, n, buffers, [&](u32 name) {
        if (gl->array_buffer == name) {
            gl->array_buffer = 0;
        }
        if (gl->default_vertex_array.element_array_buffer == name) {
            gl->default_vertex_array.element_array_buffer = 0;
        }
        for (GlVertexAttrib& attrib : gl->default_vertex_array.attribs) {
            if (attrib.buffer == name) {
                attrib.buffer = 0;
            }
        }
        for (auto& [binding_target, binding] : gl->other_buffer_bindings) {
            if (binding == name) {
                binding = 0;
            }
        }
        for (auto& [vao_name, entry] : gl->vertex_arrays.entries) {
            if (entry.object.element_array_buffer == name) {
                entry.object.element_array_buffer = 0;
            }
        }
    });
}

u8 PS4_SYSV_ABI glIsBuffer(u32 buffer) {
    GlContext* gl = RequireContext();
    return gl != nullptr && gl->buffers.IsObject(buffer) ? 1 : 0;
}

void PS4_SYSV_ABI glBindBuffer(u32 target, u32 buffer) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    u32* binding = BufferBinding(*gl, target);
    if (binding == nullptr) {
        // TODO: ES 3.0 buffer targets (copy, pixel pack/unpack, uniform, transform feedback).
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (buffer != 0) {
        gl->buffers.Bind(buffer);
    }
    *binding = buffer;
}

void PS4_SYSV_ABI glBufferData(u32 target, s64 size, const void* data, u32 usage) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    u32* binding = BufferBinding(*gl, target);
    if (binding == nullptr || !IsBufferUsage(usage)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (size < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    GlBuffer* buffer = gl->buffers.Find(*binding);
    if (buffer == nullptr) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    if (size > MAX_BUFFER_SIZE) {
        SetGlError(*gl, GL_OUT_OF_MEMORY);
        return;
    }
    buffer->mapped = false;
    buffer->data.assign(static_cast<size_t>(size), 0);
    if (data != nullptr) {
        std::memcpy(buffer->data.data(), data, static_cast<size_t>(size));
    }
    buffer->size = static_cast<u32>(size);
    buffer->usage = usage;
}

void PS4_SYSV_ABI glBufferSubData(u32 target, s64 offset, s64 size, const void* data) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    u32* binding = BufferBinding(*gl, target);
    if (binding == nullptr) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    GlBuffer* buffer = gl->buffers.Find(*binding);
    if (buffer == nullptr) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    if (offset < 0 || size < 0 || offset + size > static_cast<s64>(buffer->data.size())) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    if (size > 0 && data != nullptr) {
        std::memcpy(buffer->data.data() + offset, data, static_cast<size_t>(size));
    }
}

void PS4_SYSV_ABI glGetBufferParameteriv(u32 target, u32 pname, s32* params) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    u32* binding = BufferBinding(*gl, target);
    if (binding == nullptr || (pname != 0x8764 && pname != 0x8765)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    const GlBuffer* buffer = gl->buffers.Find(*binding);
    if (buffer == nullptr) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    *params = pname == 0x8764 ? static_cast<s32>(buffer->size) : static_cast<s32>(buffer->usage);
}

void PS4_SYSV_ABI glGenTextures(s32 n, u32* textures) {
    if (GlContext* gl = RequireContext()) {
        GenNames(*gl, gl->textures, n, textures);
    }
}

void PS4_SYSV_ABI glDeleteTextures(s32 n, const u32* textures) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    DeleteNames(*gl, gl->textures, n, textures, [&](u32 name) {
        for (auto& unit : gl->texture_bindings) {
            for (auto& [texture_target, binding] : unit) {
                if (binding == name) {
                    binding = 0;
                }
            }
        }
    });
}

u8 PS4_SYSV_ABI glIsTexture(u32 texture) {
    GlContext* gl = RequireContext();
    return gl != nullptr && texture != 0 && gl->textures.IsObject(texture) ? 1 : 0;
}

void PS4_SYSV_ABI glBindTexture(u32 target, u32 texture) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const bool is_es3_target = target == 0x806F || target == 0x8C1A; // 3D, 2D array
    if (target != GL_TEXTURE_2D && target != 0x8513 && !(gl->version >= 3 && is_es3_target)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (texture != 0) {
        GlTexture& object = gl->textures.Bind(texture);
        if (object.target != 0 && object.target != target) {
            SetGlError(*gl, GL_INVALID_OPERATION);
            return;
        }
        object.target = target;
    }
    gl->texture_bindings[gl->active_texture - GL_TEXTURE0][target] = texture;
}

void PS4_SYSV_ABI glGenFramebuffers(s32 n, u32* framebuffers) {
    if (GlContext* gl = RequireContext()) {
        GenNames(*gl, gl->framebuffers, n, framebuffers);
    }
}

void PS4_SYSV_ABI glDeleteFramebuffers(s32 n, const u32* framebuffers) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    DeleteNames(*gl, gl->framebuffers, n, framebuffers, [&](u32 name) {
        if (gl->draw_framebuffer == name) {
            gl->draw_framebuffer = 0;
        }
        if (gl->read_framebuffer == name) {
            gl->read_framebuffer = 0;
        }
    });
}

u8 PS4_SYSV_ABI glIsFramebuffer(u32 framebuffer) {
    GlContext* gl = RequireContext();
    return gl != nullptr && gl->framebuffers.IsObject(framebuffer) ? 1 : 0;
}

void PS4_SYSV_ABI glBindFramebuffer(u32 target, u32 framebuffer) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const bool es3_target = target == GL_READ_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER;
    if (target != GL_FRAMEBUFFER && !(gl->version >= 3 && es3_target)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (framebuffer != 0) {
        gl->framebuffers.Bind(framebuffer);
    }
    if (target != GL_READ_FRAMEBUFFER) {
        gl->draw_framebuffer = framebuffer;
    }
    if (target != GL_DRAW_FRAMEBUFFER) {
        gl->read_framebuffer = framebuffer;
    }
}

void PS4_SYSV_ABI glGenRenderbuffers(s32 n, u32* renderbuffers) {
    if (GlContext* gl = RequireContext()) {
        GenNames(*gl, gl->renderbuffers, n, renderbuffers);
    }
}

void PS4_SYSV_ABI glDeleteRenderbuffers(s32 n, const u32* renderbuffers) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    DeleteNames(*gl, gl->renderbuffers, n, renderbuffers, [&](u32 name) {
        if (gl->renderbuffer == name) {
            gl->renderbuffer = 0;
        }
    });
}

u8 PS4_SYSV_ABI glIsRenderbuffer(u32 renderbuffer) {
    GlContext* gl = RequireContext();
    return gl != nullptr && gl->renderbuffers.IsObject(renderbuffer) ? 1 : 0;
}

void PS4_SYSV_ABI glBindRenderbuffer(u32 target, u32 renderbuffer) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != GL_RENDERBUFFER) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (renderbuffer != 0) {
        gl->renderbuffers.Bind(renderbuffer);
    }
    gl->renderbuffer = renderbuffer;
}

void PS4_SYSV_ABI glRenderbufferStorage(u32 target, u32 internal_format, s32 width, s32 height) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != GL_RENDERBUFFER) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (width < 0 || height < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    GlRenderbuffer* renderbuffer = gl->renderbuffers.Find(gl->renderbuffer);
    if (renderbuffer == nullptr) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    // TODO: Allocate backing storage and validate the format once there is a rendering backend.
    renderbuffer->internal_format = internal_format;
    renderbuffer->width = width;
    renderbuffer->height = height;
}

void PS4_SYSV_ABI glGenVertexArrays(s32 n, u32* arrays) {
    if (GlContext* gl = RequireContext()) {
        GenNames(*gl, gl->vertex_arrays, n, arrays);
    }
}

void PS4_SYSV_ABI glDeleteVertexArrays(s32 n, const u32* arrays) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    DeleteNames(*gl, gl->vertex_arrays, n, arrays, [&](u32 name) {
        if (gl->vertex_array == name) {
            gl->vertex_array = 0;
        }
    });
}

u8 PS4_SYSV_ABI glIsVertexArray(u32 array) {
    GlContext* gl = RequireContext();
    return gl != nullptr && gl->vertex_arrays.IsObject(array) ? 1 : 0;
}

void PS4_SYSV_ABI glBindVertexArray(u32 array) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (array != 0) {
        gl->vertex_arrays.Bind(array);
    }
    gl->vertex_array = array;
}

void PS4_SYSV_ABI glGenSamplers(s32 count, u32* samplers) {
    if (GlContext* gl = RequireContext()) {
        GenNames(*gl, gl->samplers, count, samplers);
    }
}

void PS4_SYSV_ABI glDeleteSamplers(s32 count, const u32* samplers) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    DeleteNames(*gl, gl->samplers, count, samplers, [](u32) {});
}

u8 PS4_SYSV_ABI glIsSampler(u32 sampler) {
    GlContext* gl = RequireContext();
    return gl != nullptr && gl->samplers.FindGenerated(sampler) != nullptr ? 1 : 0;
}

void PS4_SYSV_ABI glGenQueries(s32 n, u32* ids) {
    if (GlContext* gl = RequireContext()) {
        GenNames(*gl, gl->queries, n, ids);
    }
}

void PS4_SYSV_ABI glDeleteQueries(s32 n, const u32* ids) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    DeleteNames(*gl, gl->queries, n, ids, [](u32) {});
}

u8 PS4_SYSV_ABI glIsQuery(u32 id) {
    GlContext* gl = RequireContext();
    return gl != nullptr && gl->queries.IsObject(id) ? 1 : 0;
}

void PS4_SYSV_ABI glGenTransformFeedbacks(s32 n, u32* ids) {
    if (GlContext* gl = RequireContext()) {
        GenNames(*gl, gl->transform_feedbacks, n, ids);
    }
}

void PS4_SYSV_ABI glDeleteTransformFeedbacks(s32 n, const u32* ids) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    DeleteNames(*gl, gl->transform_feedbacks, n, ids, [](u32) {});
}

u8 PS4_SYSV_ABI glIsTransformFeedback(u32 id) {
    GlContext* gl = RequireContext();
    return gl != nullptr && gl->transform_feedbacks.IsObject(id) ? 1 : 0;
}

u32 PS4_SYSV_ABI glCreateShader(u32 type) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return 0;
    }
    if (type != GL_VERTEX_SHADER && type != GL_FRAGMENT_SHADER) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return 0;
    }
    const u32 name = gl->next_shader_program_name++;
    gl->shaders[name].type = type;
    return name;
}

namespace {

// Looks up a shader, setting the error GL requires when the name is not a shader.
GlShader* FindShader(GlContext& gl, u32 shader) {
    const auto it = gl.shaders.find(shader);
    if (it != gl.shaders.end()) {
        return &it->second;
    }
    SetGlError(gl, gl.programs.contains(shader) ? GL_INVALID_OPERATION : GL_INVALID_VALUE);
    return nullptr;
}

GlProgram* FindProgram(GlContext& gl, u32 program) {
    const auto it = gl.programs.find(program);
    if (it != gl.programs.end()) {
        return &it->second;
    }
    SetGlError(gl, gl.shaders.contains(program) ? GL_INVALID_OPERATION : GL_INVALID_VALUE);
    return nullptr;
}

bool IsShaderAttached(const GlContext& gl, u32 shader) {
    return std::ranges::any_of(gl.programs, [&](const auto& program) {
        return std::ranges::find(program.second.shaders, shader) != program.second.shaders.end();
    });
}

} // namespace

void PS4_SYSV_ABI glShaderSource(u32 shader, s32 count, const char* const* string,
                                 const s32* length) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    GlShader* object = FindShader(*gl, shader);
    if (object == nullptr) {
        return;
    }
    if (count < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    object->source.clear();
    for (s32 i = 0; i < count; i++) {
        if (length != nullptr && length[i] >= 0) {
            object->source.append(string[i], static_cast<size_t>(length[i]));
        } else {
            object->source.append(string[i]);
        }
    }
}

void PS4_SYSV_ABI glCompileShader(u32 shader) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    GlShader* object = FindShader(*gl, shader);
    if (object == nullptr) {
        return;
    }
    object->compiled = false;
    object->info_log = SHADER_COMPILER_UNSUPPORTED;
}

void PS4_SYSV_ABI glShaderBinary(s32 count, const u32* shaders, [[maybe_unused]] u32 binary_format,
                                 const void* binary, s32 length) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    // Piglet does not look at binary_format.
    if (count < 0 || length < 0 || binary == nullptr) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    for (s32 i = 0; i < count; i++) {
        if (!gl->shaders.contains(shaders[i])) {
            SetGlError(*gl, GL_INVALID_VALUE);
            return;
        }
    }
    const auto* bytes = static_cast<const u8*>(binary);
    for (s32 i = 0; i < count; i++) {
        GlShader& object = gl->shaders.at(shaders[i]);
        // TODO: Piglet decodes the binary here and fails when it is not a valid shader. An empty
        // binary is the only case known to be invalid.
        if (length == 0) {
            SetGlError(*gl, GL_INVALID_OPERATION);
            LOG_ERROR(Lib_Piglet, "Failed to load shader binary");
            continue;
        }
        object.binary.assign(bytes, bytes + length);
        object.compiled = true;
        object.info_log.clear();
    }
}

void PS4_SYSV_ABI glDeleteShader(u32 shader) {
    GlContext* gl = RequireContext();
    if (gl == nullptr || shader == 0) {
        return;
    }
    GlShader* object = FindShader(*gl, shader);
    if (object == nullptr) {
        return;
    }
    if (IsShaderAttached(*gl, shader)) {
        object->delete_pending = true;
    } else {
        gl->shaders.erase(shader);
    }
}

u8 PS4_SYSV_ABI glIsShader(u32 shader) {
    GlContext* gl = RequireContext();
    return gl != nullptr && gl->shaders.contains(shader) ? 1 : 0;
}

void PS4_SYSV_ABI glGetShaderiv(u32 shader, u32 pname, s32* params) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const GlShader* object = FindShader(*gl, shader);
    if (object == nullptr) {
        return;
    }
    switch (pname) {
    case 0x8B4F: // GL_SHADER_TYPE
        *params = static_cast<s32>(object->type);
        break;
    case 0x8B80: // GL_DELETE_STATUS
        *params = object->delete_pending ? 1 : 0;
        break;
    case 0x8B81: // GL_COMPILE_STATUS
        *params = object->compiled ? 1 : 0;
        break;
    case 0x8B84: // GL_INFO_LOG_LENGTH
        *params = object->info_log.empty() ? 0 : static_cast<s32>(object->info_log.size() + 1);
        break;
    case 0x8B88: // GL_SHADER_SOURCE_LENGTH
        *params = object->source.empty() ? 0 : static_cast<s32>(object->source.size() + 1);
        break;
    default:
        SetGlError(*gl, GL_INVALID_ENUM);
        break;
    }
}

void PS4_SYSV_ABI glGetShaderInfoLog(u32 shader, s32 buf_size, s32* length, char* info_log) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const GlShader* object = FindShader(*gl, shader);
    if (object == nullptr) {
        return;
    }
    if (buf_size < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    CopyString(object->info_log, buf_size, length, info_log);
}

void PS4_SYSV_ABI glGetShaderSource(u32 shader, s32 buf_size, s32* length, char* source) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const GlShader* object = FindShader(*gl, shader);
    if (object == nullptr) {
        return;
    }
    if (buf_size < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    CopyString(object->source, buf_size, length, source);
}

void PS4_SYSV_ABI glReleaseShaderCompiler() {}

u32 PS4_SYSV_ABI glCreateProgram() {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return 0;
    }
    const u32 name = gl->next_shader_program_name++;
    gl->programs[name];
    return name;
}

void PS4_SYSV_ABI glAttachShader(u32 program, u32 shader) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    GlProgram* prog = FindProgram(*gl, program);
    if (prog == nullptr || FindShader(*gl, shader) == nullptr) {
        return;
    }
    if (std::ranges::find(prog->shaders, shader) != prog->shaders.end()) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    prog->shaders.push_back(shader);
}

void PS4_SYSV_ABI glDetachShader(u32 program, u32 shader) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    GlProgram* prog = FindProgram(*gl, program);
    if (prog == nullptr || FindShader(*gl, shader) == nullptr) {
        return;
    }
    const auto it = std::ranges::find(prog->shaders, shader);
    if (it == prog->shaders.end()) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    prog->shaders.erase(it);
    const auto shader_it = gl->shaders.find(shader);
    if (shader_it != gl->shaders.end() && shader_it->second.delete_pending &&
        !IsShaderAttached(*gl, shader)) {
        gl->shaders.erase(shader_it);
    }
}

void PS4_SYSV_ABI glLinkProgram(u32 program) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    GlProgram* prog = FindProgram(*gl, program);
    if (prog == nullptr) {
        return;
    }
    // TODO: Link the shader binaries once there is a rendering backend. For now a program links
    // when it has a compiled vertex shader and a compiled fragment shader.
    bool has_vertex = false;
    bool has_fragment = false;
    for (const u32 name : prog->shaders) {
        const GlShader& shader = gl->shaders.at(name);
        if (shader.compiled) {
            has_vertex |= shader.type == GL_VERTEX_SHADER;
            has_fragment |= shader.type == GL_FRAGMENT_SHADER;
        }
    }
    prog->linked = has_vertex && has_fragment;
    prog->info_log =
        prog->linked ? "" : "Link failed: missing compiled vertex or fragment shader\n";
}

void PS4_SYSV_ABI glUseProgram(u32 program) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (program != 0) {
        const GlProgram* prog = FindProgram(*gl, program);
        if (prog == nullptr) {
            return;
        }
        if (!prog->linked) {
            SetGlError(*gl, GL_INVALID_OPERATION);
            return;
        }
    }
    gl->current_program = program;
}

void PS4_SYSV_ABI glDeleteProgram(u32 program) {
    GlContext* gl = RequireContext();
    if (gl == nullptr || program == 0) {
        return;
    }
    GlProgram* prog = FindProgram(*gl, program);
    if (prog == nullptr) {
        return;
    }
    if (gl->current_program == program) {
        prog->delete_pending = true;
        return;
    }
    gl->programs.erase(program);
}

u8 PS4_SYSV_ABI glIsProgram(u32 program) {
    GlContext* gl = RequireContext();
    return gl != nullptr && gl->programs.contains(program) ? 1 : 0;
}

void PS4_SYSV_ABI glGetProgramiv(u32 program, u32 pname, s32* params) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const GlProgram* prog = FindProgram(*gl, program);
    if (prog == nullptr) {
        return;
    }
    switch (pname) {
    case 0x8B80: // GL_DELETE_STATUS
        *params = prog->delete_pending ? 1 : 0;
        break;
    case 0x8B82: // GL_LINK_STATUS
        *params = prog->linked ? 1 : 0;
        break;
    case 0x8B83: // GL_VALIDATE_STATUS
        *params = prog->validated ? 1 : 0;
        break;
    case 0x8B84: // GL_INFO_LOG_LENGTH
        *params = prog->info_log.empty() ? 0 : static_cast<s32>(prog->info_log.size() + 1);
        break;
    case 0x8B85: // GL_ATTACHED_SHADERS
        *params = static_cast<s32>(prog->shaders.size());
        break;
    default:
        // TODO: Active attribute and uniform queries need program reflection.
        SetGlError(*gl, GL_INVALID_ENUM);
        break;
    }
}

void PS4_SYSV_ABI glGetProgramInfoLog(u32 program, s32 buf_size, s32* length, char* info_log) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const GlProgram* prog = FindProgram(*gl, program);
    if (prog == nullptr) {
        return;
    }
    if (buf_size < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    CopyString(prog->info_log, buf_size, length, info_log);
}

void PS4_SYSV_ABI glGetAttachedShaders(u32 program, s32 max_count, s32* count, u32* shaders) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const GlProgram* prog = FindProgram(*gl, program);
    if (prog == nullptr) {
        return;
    }
    if (max_count < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    const s32 written = std::min(max_count, static_cast<s32>(prog->shaders.size()));
    std::copy_n(prog->shaders.begin(), written, shaders);
    if (count != nullptr) {
        *count = written;
    }
}

u64 PS4_SYSV_ABI glFenceSync(u32 condition, u32 flags) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return 0;
    }
    if (condition != 0x9117) { // GL_SYNC_GPU_COMMANDS_COMPLETE
        SetGlError(*gl, GL_INVALID_ENUM);
        return 0;
    }
    if (flags != 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return 0;
    }
    const u64 sync = gl->next_sync++;
    gl->syncs[sync];
    return sync;
}

u8 PS4_SYSV_ABI glIsSync(u64 sync) {
    GlContext* gl = RequireContext();
    return gl != nullptr && gl->syncs.contains(sync) ? 1 : 0;
}

void PS4_SYSV_ABI glDeleteSync(u64 sync) {
    GlContext* gl = RequireContext();
    if (gl == nullptr || sync == 0) {
        return;
    }
    if (gl->syncs.erase(sync) == 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
    }
}

u32 PS4_SYSV_ABI glClientWaitSync(u64 sync, u32 flags, [[maybe_unused]] u64 timeout) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return 0x911D; // GL_WAIT_FAILED
    }
    if (!gl->syncs.contains(sync) || (flags & ~1u) != 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return 0x911D;
    }
    // Without a rendering backend there is never pending GPU work, so fences are always signaled.
    return 0x911A; // GL_ALREADY_SIGNALED
}

void PS4_SYSV_ABI glWaitSync(u64 sync, u32 flags, [[maybe_unused]] u64 timeout) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!gl->syncs.contains(sync) || flags != 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
    }
}

void PS4_SYSV_ABI glGetSynciv(u64 sync, u32 pname, s32 buf_size, s32* length, s32* values) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const auto it = gl->syncs.find(sync);
    if (it == gl->syncs.end() || buf_size < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    s32 value = 0;
    switch (pname) {
    case 0x9112:        // GL_OBJECT_TYPE
        value = 0x9116; // GL_SYNC_FENCE
        break;
    case 0x9113: // GL_SYNC_CONDITION
        value = static_cast<s32>(it->second.condition);
        break;
    case 0x9114:        // GL_SYNC_STATUS
        value = 0x9119; // GL_SIGNALED
        break;
    case 0x9115: // GL_SYNC_FLAGS
        break;
    default:
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (buf_size > 0) {
        values[0] = value;
    }
    if (length != nullptr) {
        *length = buf_size > 0 ? 1 : 0;
    }
}

namespace {

constexpr u32 GL_INVALID_FRAMEBUFFER_OPERATION = 0x0506;
constexpr u32 GL_TEXTURE_CUBE_MAP = 0x8513;
constexpr u32 GL_TEXTURE_3D = 0x806F;
constexpr u32 GL_TEXTURE_2D_ARRAY = 0x8C1A;
constexpr u32 GL_FRAMEBUFFER_COMPLETE = 0x8CD5;
constexpr u32 GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT = 0x8CD6;
constexpr u32 GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT = 0x8CD7;
constexpr u32 GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS = 0x8CD9;
constexpr u32 GL_COLOR_ATTACHMENT0 = 0x8CE0;
constexpr u32 GL_DEPTH_ATTACHMENT = 0x8D00;
constexpr u32 GL_STENCIL_ATTACHMENT = 0x8D20;
constexpr u32 GL_DEPTH_STENCIL_ATTACHMENT = 0x821A;
constexpr u32 GL_INVALID_INDEX = 0xFFFFFFFF;
constexpr s32 MAX_TEXTURE_SIZE = 4096;
constexpr s32 MAX_CUBE_MAP_TEXTURE_SIZE = 2048;
constexpr s32 MAX_3D_TEXTURE_SIZE = 256;
constexpr s32 MAX_DRAW_BUFFERS = 4;

GlVertexArray& CurrentVertexArray(GlContext& gl) {
    if (gl.vertex_array != 0) {
        if (GlVertexArray* vao = gl.vertex_arrays.Find(gl.vertex_array)) {
            return *vao;
        }
    }
    return gl.default_vertex_array;
}

GlTransformFeedback& CurrentTransformFeedback(GlContext& gl) {
    if (gl.transform_feedback != 0) {
        if (GlTransformFeedback* tf = gl.transform_feedbacks.Find(gl.transform_feedback)) {
            return *tf;
        }
    }
    return gl.default_transform_feedback;
}

// Texture bound to a target on the active texture unit. Texture 0 is the default texture.
GlTexture* BoundTexture(GlContext& gl, u32 target) {
    const auto& bindings = gl.texture_bindings[gl.active_texture - GL_TEXTURE0];
    const auto it = bindings.find(target);
    return &gl.textures.Bind(it != bindings.end() ? it->second : 0);
}

bool IsTexture2DTarget(u32 target) {
    return target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP;
}

bool IsTexture3DTarget(const GlContext& gl, u32 target) {
    return gl.version >= 3 && (target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY);
}

bool IsCubeFace(u32 target) {
    return target >= 0x8515 && target <= 0x851A;
}

bool IsTextureFilter(u32 value, bool mipmaps) {
    return value == 0x2600 || value == 0x2601 || (mipmaps && value >= 0x2700 && value <= 0x2703);
}

bool IsWrapMode(u32 value) {
    return value == 0x2901 || value == 0x812F || value == 0x8370;
}

bool IsTextureFormat(const GlContext& gl, u32 format) {
    switch (format) {
    case 0x1902: // GL_DEPTH_COMPONENT
    case 0x1906: // GL_ALPHA
    case 0x1907: // GL_RGB
    case 0x1908: // GL_RGBA
    case 0x1909: // GL_LUMINANCE
    case 0x190A: // GL_LUMINANCE_ALPHA
    case 0x80E1: // GL_BGRA_EXT
        return true;
    case 0x1903: // GL_RED
    case 0x8227: // GL_RG
    case 0x84F9: // GL_DEPTH_STENCIL
    case 0x8D94: // GL_RED_INTEGER
    case 0x8228: // GL_RG_INTEGER
    case 0x8D98: // GL_RGB_INTEGER
    case 0x8D99: // GL_RGBA_INTEGER
        return gl.version >= 3;
    default:
        return false;
    }
}

bool IsPixelType(u32 type) {
    switch (type) {
    case 0x1400: // GL_BYTE
    case 0x1401: // GL_UNSIGNED_BYTE
    case 0x1402: // GL_SHORT
    case 0x1403: // GL_UNSIGNED_SHORT
    case 0x1404: // GL_INT
    case 0x1405: // GL_UNSIGNED_INT
    case 0x1406: // GL_FLOAT
    case 0x140B: // GL_HALF_FLOAT
    case 0x8D61: // GL_HALF_FLOAT_OES
    case 0x8363: // GL_UNSIGNED_SHORT_5_6_5
    case 0x8033: // GL_UNSIGNED_SHORT_4_4_4_4
    case 0x8034: // GL_UNSIGNED_SHORT_5_5_5_1
    case 0x84FA: // GL_UNSIGNED_INT_24_8
    case 0x8368: // GL_UNSIGNED_INT_2_10_10_10_REV
    case 0x8C3B: // GL_UNSIGNED_INT_10F_11F_11F_REV
    case 0x8C3E: // GL_UNSIGNED_INT_5_9_9_9_REV
    case 0x8DAD: // GL_FLOAT_32_UNSIGNED_INT_24_8_REV
        return true;
    default:
        return false;
    }
}

bool IsCompressedFormat(u32 format) {
    return std::ranges::find(COMPRESSED_FORMATS, format) != COMPRESSED_FORMATS.end();
}

// Largest texture dimension of a target.
s32 MaxTextureDimension(u32 target) {
    if (target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY) {
        return MAX_3D_TEXTURE_SIZE;
    }
    return target == GL_TEXTURE_CUBE_MAP || IsCubeFace(target) ? MAX_CUBE_MAP_TEXTURE_SIZE
                                                               : MAX_TEXTURE_SIZE;
}

// Face or layer key of a texture image target.
u32 LevelKey(u32 target) {
    return IsCubeFace(target) ? target : (target == GL_TEXTURE_CUBE_MAP ? 0 : target);
}

u32 BindingTarget(u32 target) {
    return IsCubeFace(target) ? GL_TEXTURE_CUBE_MAP : target;
}

s32 MaxLevels(s32 size) {
    s32 levels = 1;
    while (size > 1) {
        size >>= 1;
        levels++;
    }
    return levels;
}

// Sets a texture level. Returns false after raising the matching error.
bool SetTextureLevel(GlContext& gl, u32 target, s32 level, s32 width, s32 height, s32 depth,
                     u32 internal_format, bool compressed) {
    GlTexture* texture = BoundTexture(gl, BindingTarget(target));
    if (level < 0 || level >= MaxLevels(MaxTextureDimension(target)) || width < 0 || height < 0 ||
        depth < 0 || width > MaxTextureDimension(target) || height > MaxTextureDimension(target)) {
        SetGlError(gl, GL_INVALID_VALUE);
        return false;
    }
    if (texture->immutable) {
        SetGlError(gl, GL_INVALID_OPERATION);
        return false;
    }
    texture->target = BindingTarget(target);
    texture->levels[{LevelKey(target), level}] = {width, height, depth, internal_format,
                                                  compressed};
    return true;
}

// Validates a sub image update against the level it targets.
bool CheckSubImage(GlContext& gl, u32 target, s32 level, s32 xoffset, s32 yoffset, s32 zoffset,
                   s32 width, s32 height, s32 depth) {
    GlTexture* texture = BoundTexture(gl, BindingTarget(target));
    const auto it = texture->levels.find({LevelKey(target), level});
    if (level < 0 || width < 0 || height < 0 || depth < 0 || xoffset < 0 || yoffset < 0 ||
        zoffset < 0) {
        SetGlError(gl, GL_INVALID_VALUE);
        return false;
    }
    if (it == texture->levels.end()) {
        SetGlError(gl, GL_INVALID_OPERATION);
        return false;
    }
    const GlTextureLevel& image = it->second;
    if (xoffset + width > image.width || yoffset + height > image.height ||
        zoffset + depth > std::max(image.depth, 1)) {
        SetGlError(gl, GL_INVALID_VALUE);
        return false;
    }
    return true;
}

s32 AttachmentSize(GlContext& gl, const GlFramebufferAttachment& attachment, bool width) {
    if (attachment.type == GL_RENDERBUFFER) {
        const GlRenderbuffer* renderbuffer = gl.renderbuffers.Find(attachment.name);
        return renderbuffer == nullptr ? 0 : (width ? renderbuffer->width : renderbuffer->height);
    }
    GlTexture* texture = gl.textures.Find(attachment.name);
    if (texture == nullptr) {
        return 0;
    }
    const auto it = texture->levels.find({LevelKey(attachment.texture_target), attachment.level});
    return it == texture->levels.end() ? 0 : (width ? it->second.width : it->second.height);
}

u32 FramebufferStatus(GlContext& gl, u32 name) {
    if (name == 0) {
        return GL_FRAMEBUFFER_COMPLETE;
    }
    const GlFramebuffer* framebuffer = gl.framebuffers.Find(name);
    if (framebuffer == nullptr) {
        return GL_FRAMEBUFFER_COMPLETE;
    }
    s32 width = -1;
    s32 height = -1;
    bool any = false;
    for (const auto& [point, attachment] : framebuffer->attachments) {
        if (attachment.type == 0) {
            continue;
        }
        any = true;
        const s32 w = AttachmentSize(gl, attachment, true);
        const s32 h = AttachmentSize(gl, attachment, false);
        if (w <= 0 || h <= 0) {
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }
        if (width != -1 && (w != width || h != height)) {
            return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
        }
        width = w;
        height = h;
    }
    return any ? GL_FRAMEBUFFER_COMPLETE : GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
}

// Rendering commands fail while the draw framebuffer is incomplete.
bool CheckDrawFramebuffer(GlContext& gl) {
    if (FramebufferStatus(gl, gl.draw_framebuffer) != GL_FRAMEBUFFER_COMPLETE) {
        SetGlError(gl, GL_INVALID_FRAMEBUFFER_OPERATION);
        return false;
    }
    return true;
}

bool IsDrawMode(u32 mode) {
    return mode <= 6;
}

bool IsIndexType(u32 type) {
    return type == 0x1401 || type == 0x1403 || type == 0x1405;
}

// Shared checks of the glDraw* family. The commands draw nothing without a rendering backend.
bool CheckDraw(GlContext& gl, u32 mode, s32 count) {
    if (!IsDrawMode(mode)) {
        SetGlError(gl, GL_INVALID_ENUM);
        return false;
    }
    if (count < 0) {
        SetGlError(gl, GL_INVALID_VALUE);
        return false;
    }
    if (gl.current_program == 0) {
        SetGlError(gl, GL_INVALID_OPERATION);
        return false;
    }
    return CheckDrawFramebuffer(gl);
}

bool CheckVertexIndex(GlContext& gl, u32 index) {
    if (index >= MAX_VERTEX_ATTRIBS) {
        SetGlError(gl, GL_INVALID_VALUE);
        return false;
    }
    return true;
}

// Stores a uniform value. Piglet's program reflection is not available, so any location that is
// not -1 is accepted.
template <typename T>
void StoreUniform(s32 location, s32 count, const T* values, u32 components, bool es3_only = false) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if ((es3_only && gl->version < 3) || gl->current_program == 0) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    if (location == -1) {
        return;
    }
    if (count < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    GlUniformValue& uniform = gl->programs[gl->current_program].uniforms[location];
    uniform.is_float = std::is_same_v<T, float>;
    uniform.count = components;
    for (u32 i = 0; i < components && i < 16; i++) {
        if constexpr (std::is_same_v<T, float>) {
            uniform.floats[i] = values[i];
        } else {
            uniform.ints[i] = static_cast<s32>(values[i]);
        }
    }
}

void StoreUniformMatrix(s32 location, s32 count, u8 transpose, const float* values, u32 components,
                        bool es3_only = false) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (transpose != 0 && gl->version < 3) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    StoreUniform(location, count, values, components, es3_only);
}

template <typename T>
void GetUniform(u32 program, s32 location, T* params) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const GlProgram* prog = FindProgram(*gl, program);
    if (prog == nullptr) {
        return;
    }
    const auto it = prog->uniforms.find(location);
    if (!prog->linked || it == prog->uniforms.end()) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    for (u32 i = 0; i < it->second.count; i++) {
        params[i] = it->second.is_float ? static_cast<T>(it->second.floats[i])
                                        : static_cast<T>(it->second.ints[i]);
    }
}

template <typename T>
void SetVertexAttrib(u32 index, std::array<T, 4> value) {
    GlContext* gl = RequireContext();
    if (gl == nullptr || !CheckVertexIndex(*gl, index)) {
        return;
    }
    GlVertexAttrib& attrib = CurrentVertexArray(*gl).attribs[index];
    if constexpr (std::is_same_v<T, float>) {
        attrib.current_float = value;
        attrib.current_is_int = false;
    } else {
        attrib.current_int = {static_cast<s32>(value[0]), static_cast<s32>(value[1]),
                              static_cast<s32>(value[2]), static_cast<s32>(value[3])};
        attrib.current_is_int = true;
    }
}

bool IsQueryTarget(u32 target) {
    return target == 0x8C2F || target == 0x8D6A || target == 0x8C88 || target == 0x88BF;
}

bool IsSamplerParam(u32 pname) {
    switch (pname) {
    case 0x2800: // GL_TEXTURE_MAG_FILTER
    case 0x2801: // GL_TEXTURE_MIN_FILTER
    case 0x2802: // GL_TEXTURE_WRAP_S
    case 0x2803: // GL_TEXTURE_WRAP_T
    case 0x8072: // GL_TEXTURE_WRAP_R
    case 0x884C: // GL_TEXTURE_COMPARE_MODE
    case 0x884D: // GL_TEXTURE_COMPARE_FUNC
    case 0x813A: // GL_TEXTURE_MIN_LOD
    case 0x813B: // GL_TEXTURE_MAX_LOD
    case 0x84FE: // GL_TEXTURE_MAX_ANISOTROPY_EXT
        return true;
    default:
        return false;
    }
}

bool IsTextureParam(const GlContext& gl, u32 pname) {
    switch (pname) {
    case 0x813C: // GL_TEXTURE_BASE_LEVEL
    case 0x813D: // GL_TEXTURE_MAX_LEVEL
    case 0x8E42: // GL_TEXTURE_SWIZZLE_R
    case 0x8E43: // GL_TEXTURE_SWIZZLE_G
    case 0x8E44: // GL_TEXTURE_SWIZZLE_B
    case 0x8E45: // GL_TEXTURE_SWIZZLE_A
        return gl.version >= 3;
    default:
        return IsSamplerParam(pname) && (gl.version >= 3 || (pname < 0x8072 || pname == 0x84FE));
    }
}

bool IsFloatParam(u32 pname) {
    return pname == 0x813A || pname == 0x813B || pname == 0x84FE;
}

// Default value of a texture or sampler parameter, from Piglet's texture initializer.
double DefaultTextureParam(u32 pname) {
    switch (pname) {
    case 0x2800:
        return 0x2601; // GL_LINEAR
    case 0x2801:
        return 0x2702; // GL_NEAREST_MIPMAP_LINEAR
    case 0x2802:
    case 0x2803:
    case 0x8072:
        return 0x2901; // GL_REPEAT
    case 0x884D:
        return 0x0203; // GL_LEQUAL
    case 0x813D:
        return 1000;
    case 0x813A:
        return -1000;
    case 0x813B:
        return 1000;
    case 0x84FE:
        return 1;
    case 0x8E42:
        return 0x1903; // GL_RED
    case 0x8E43:
        return 0x1904; // GL_GREEN
    case 0x8E44:
        return 0x1905; // GL_BLUE
    case 0x8E45:
        return 0x1906; // GL_ALPHA
    default:
        return 0; // base level, compare mode
    }
}

// Validates and stores a texture or sampler parameter.
template <typename Params>
void SetTextureParam(GlContext& gl, Params& params, u32 pname, double value) {
    if (IsFloatParam(pname)) {
        params.float_params[pname] = static_cast<float>(value);
        return;
    }
    const u32 enum_value = static_cast<u32>(value);
    bool valid = true;
    switch (pname) {
    case 0x2800:
        valid = IsTextureFilter(enum_value, false);
        break;
    case 0x2801:
        valid = IsTextureFilter(enum_value, true);
        break;
    case 0x2802:
    case 0x2803:
    case 0x8072:
        valid = IsWrapMode(enum_value);
        break;
    case 0x884C:
        valid = enum_value == 0 || enum_value == 0x884E;
        break;
    case 0x884D:
        valid = IsCompareFunc(enum_value);
        break;
    default:
        break;
    }
    if (!valid) {
        SetGlError(gl, GL_INVALID_ENUM);
        return;
    }
    params.int_params[pname] = static_cast<s32>(value);
}

template <typename Params>
double GetTextureParam(const Params& params, u32 pname) {
    if (IsFloatParam(pname)) {
        const auto it = params.float_params.find(pname);
        return it != params.float_params.end() ? it->second : DefaultTextureParam(pname);
    }
    const auto it = params.int_params.find(pname);
    return it != params.int_params.end() ? it->second : DefaultTextureParam(pname);
}

// Reports the formats that can be read back with glReadPixels, in bytes per pixel.
s32 PixelSize(u32 format, u32 type) {
    if (type != 0x1401) {
        return 0;
    }
    switch (format) {
    case 0x1908: // GL_RGBA
    case 0x80E1: // GL_BGRA_EXT
        return 4;
    case 0x1907: // GL_RGB
        return 3;
    default:
        return 0;
    }
}

} // namespace

void PS4_SYSV_ABI glEnableVertexAttribArray(u32 index) {
    GlContext* gl = RequireContext();
    if (gl != nullptr && CheckVertexIndex(*gl, index)) {
        CurrentVertexArray(*gl).attribs[index].enabled = true;
    }
}

void PS4_SYSV_ABI glDisableVertexAttribArray(u32 index) {
    GlContext* gl = RequireContext();
    if (gl != nullptr && CheckVertexIndex(*gl, index)) {
        CurrentVertexArray(*gl).attribs[index].enabled = false;
    }
}

void PS4_SYSV_ABI glVertexAttribPointer(u32 index, s32 size, u32 type, u8 normalized, s32 stride,
                                        const void* pointer) {
    GlContext* gl = RequireContext();
    if (gl == nullptr || !CheckVertexIndex(*gl, index)) {
        return;
    }
    bool valid_type = type == 0x1400 || type == 0x1401 || type == 0x1402 || type == 0x1403 ||
                      type == 0x1406 || type == 0x140C;
    if (gl->version >= 3) {
        valid_type |=
            type == 0x1404 || type == 0x1405 || type == 0x140B || type == 0x8D9F || type == 0x8368;
    }
    if (!valid_type) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (size < 1 || size > 4 || stride < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    GlVertexAttrib& attrib = CurrentVertexArray(*gl).attribs[index];
    attrib.size = size;
    attrib.type = type;
    attrib.normalized = normalized != 0;
    attrib.integer = false;
    attrib.stride = stride;
    attrib.pointer = pointer;
    attrib.buffer = gl->array_buffer;
}

void PS4_SYSV_ABI glVertexAttribIPointer(u32 index, s32 size, u32 type, s32 stride,
                                         const void* pointer) {
    GlContext* gl = RequireContext();
    if (gl == nullptr || !CheckVertexIndex(*gl, index)) {
        return;
    }
    const bool valid_type = type >= 0x1400 && type <= 0x1405;
    if (!valid_type) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (size < 1 || size > 4 || stride < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    GlVertexAttrib& attrib = CurrentVertexArray(*gl).attribs[index];
    attrib.size = size;
    attrib.type = type;
    attrib.normalized = false;
    attrib.integer = true;
    attrib.stride = stride;
    attrib.pointer = pointer;
    attrib.buffer = gl->array_buffer;
}

void PS4_SYSV_ABI glVertexAttribDivisor(u32 index, u32 divisor) {
    GlContext* gl = RequireContext();
    if (gl != nullptr && CheckVertexIndex(*gl, index)) {
        CurrentVertexArray(*gl).attribs[index].divisor = divisor;
    }
}

void PS4_SYSV_ABI glVertexAttribDivisorEXT(u32 index, u32 divisor) {
    glVertexAttribDivisor(index, divisor);
}

void PS4_SYSV_ABI glVertexAttrib1f(u32 index, float x) {
    SetVertexAttrib<float>(index, {x, 0.0f, 0.0f, 1.0f});
}

void PS4_SYSV_ABI glVertexAttrib2f(u32 index, float x, float y) {
    SetVertexAttrib<float>(index, {x, y, 0.0f, 1.0f});
}

void PS4_SYSV_ABI glVertexAttrib3f(u32 index, float x, float y, float z) {
    SetVertexAttrib<float>(index, {x, y, z, 1.0f});
}

void PS4_SYSV_ABI glVertexAttrib4f(u32 index, float x, float y, float z, float w) {
    SetVertexAttrib<float>(index, {x, y, z, w});
}

void PS4_SYSV_ABI glVertexAttrib1fv(u32 index, const float* v) {
    glVertexAttrib1f(index, v[0]);
}

void PS4_SYSV_ABI glVertexAttrib2fv(u32 index, const float* v) {
    glVertexAttrib2f(index, v[0], v[1]);
}

void PS4_SYSV_ABI glVertexAttrib3fv(u32 index, const float* v) {
    glVertexAttrib3f(index, v[0], v[1], v[2]);
}

void PS4_SYSV_ABI glVertexAttrib4fv(u32 index, const float* v) {
    glVertexAttrib4f(index, v[0], v[1], v[2], v[3]);
}

void PS4_SYSV_ABI glVertexAttribI4i(u32 index, s32 x, s32 y, s32 z, s32 w) {
    SetVertexAttrib<s32>(index, {x, y, z, w});
}

void PS4_SYSV_ABI glVertexAttribI4ui(u32 index, u32 x, u32 y, u32 z, u32 w) {
    SetVertexAttrib<u32>(index, {x, y, z, w});
}

void PS4_SYSV_ABI glVertexAttribI4iv(u32 index, const s32* v) {
    glVertexAttribI4i(index, v[0], v[1], v[2], v[3]);
}

void PS4_SYSV_ABI glVertexAttribI4uiv(u32 index, const u32* v) {
    glVertexAttribI4ui(index, v[0], v[1], v[2], v[3]);
}

// Shared implementation of the glGetVertexAttrib* queries.
static void GetVertexAttrib(u32 index, u32 pname, std::array<double, 4>& out, u32& count,
                            bool integer_query) {
    GlContext* gl = RequireContext();
    count = 0;
    if (gl == nullptr || !CheckVertexIndex(*gl, index)) {
        return;
    }
    const GlVertexAttrib& attrib = CurrentVertexArray(*gl).attribs[index];
    switch (pname) {
    case 0x8622: // GL_VERTEX_ATTRIB_ARRAY_ENABLED
        out[0] = attrib.enabled ? 1 : 0;
        break;
    case 0x8623: // GL_VERTEX_ATTRIB_ARRAY_SIZE
        out[0] = attrib.size;
        break;
    case 0x8624: // GL_VERTEX_ATTRIB_ARRAY_STRIDE
        out[0] = attrib.stride;
        break;
    case 0x8625: // GL_VERTEX_ATTRIB_ARRAY_TYPE
        out[0] = attrib.type;
        break;
    case 0x886A: // GL_VERTEX_ATTRIB_ARRAY_NORMALIZED
        out[0] = attrib.normalized ? 1 : 0;
        break;
    case 0x889F: // GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING
        out[0] = attrib.buffer;
        break;
    case 0x88FE: // GL_VERTEX_ATTRIB_ARRAY_DIVISOR
        out[0] = attrib.divisor;
        break;
    case 0x88FD: // GL_VERTEX_ATTRIB_ARRAY_INTEGER
        out[0] = attrib.integer ? 1 : 0;
        break;
    case 0x8626: // GL_CURRENT_VERTEX_ATTRIB
        for (u32 i = 0; i < 4; i++) {
            out[i] = attrib.current_is_int ? attrib.current_int[i] : attrib.current_float[i];
        }
        count = 4;
        return;
    default:
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    count = 1;
}

void PS4_SYSV_ABI glGetVertexAttribfv(u32 index, u32 pname, float* params) {
    std::array<double, 4> values{};
    u32 count = 0;
    GetVertexAttrib(index, pname, values, count, false);
    for (u32 i = 0; i < count; i++) {
        params[i] = static_cast<float>(values[i]);
    }
}

void PS4_SYSV_ABI glGetVertexAttribiv(u32 index, u32 pname, s32* params) {
    std::array<double, 4> values{};
    u32 count = 0;
    GetVertexAttrib(index, pname, values, count, false);
    for (u32 i = 0; i < count; i++) {
        params[i] = ToInteger(values[i]);
    }
}

void PS4_SYSV_ABI glGetVertexAttribIiv(u32 index, u32 pname, s32* params) {
    glGetVertexAttribiv(index, pname, params);
}

void PS4_SYSV_ABI glGetVertexAttribIuiv(u32 index, u32 pname, u32* params) {
    std::array<double, 4> values{};
    u32 count = 0;
    GetVertexAttrib(index, pname, values, count, true);
    for (u32 i = 0; i < count; i++) {
        params[i] = static_cast<u32>(values[i]);
    }
}

void PS4_SYSV_ABI glGetVertexAttribPointerv(u32 index, u32 pname, void** pointer) {
    GlContext* gl = RequireContext();
    if (gl == nullptr || !CheckVertexIndex(*gl, index)) {
        return;
    }
    if (pname != 0x8645) { // GL_VERTEX_ATTRIB_ARRAY_POINTER
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    *pointer = const_cast<void*>(CurrentVertexArray(*gl).attribs[index].pointer);
}

void PS4_SYSV_ABI glUniform1f(s32 location, float x) {
    const float v[] = {x};
    StoreUniform(location, 1, v, 1);
}

void PS4_SYSV_ABI glUniform2f(s32 location, float x, float y) {
    const float v[] = {x, y};
    StoreUniform(location, 1, v, 2);
}

void PS4_SYSV_ABI glUniform3f(s32 location, float x, float y, float z) {
    const float v[] = {x, y, z};
    StoreUniform(location, 1, v, 3);
}

void PS4_SYSV_ABI glUniform4f(s32 location, float x, float y, float z, float w) {
    const float v[] = {x, y, z, w};
    StoreUniform(location, 1, v, 4);
}

void PS4_SYSV_ABI glUniform1i(s32 location, s32 x) {
    const s32 v[] = {x};
    StoreUniform(location, 1, v, 1);
}

void PS4_SYSV_ABI glUniform2i(s32 location, s32 x, s32 y) {
    const s32 v[] = {x, y};
    StoreUniform(location, 1, v, 2);
}

void PS4_SYSV_ABI glUniform3i(s32 location, s32 x, s32 y, s32 z) {
    const s32 v[] = {x, y, z};
    StoreUniform(location, 1, v, 3);
}

void PS4_SYSV_ABI glUniform4i(s32 location, s32 x, s32 y, s32 z, s32 w) {
    const s32 v[] = {x, y, z, w};
    StoreUniform(location, 1, v, 4);
}

void PS4_SYSV_ABI glUniform1ui(s32 location, u32 x) {
    const u32 v[] = {x};
    StoreUniform(location, 1, v, 1, true);
}

void PS4_SYSV_ABI glUniform2ui(s32 location, u32 x, u32 y) {
    const u32 v[] = {x, y};
    StoreUniform(location, 1, v, 2, true);
}

void PS4_SYSV_ABI glUniform3ui(s32 location, u32 x, u32 y, u32 z) {
    const u32 v[] = {x, y, z};
    StoreUniform(location, 1, v, 3, true);
}

void PS4_SYSV_ABI glUniform4ui(s32 location, u32 x, u32 y, u32 z, u32 w) {
    const u32 v[] = {x, y, z, w};
    StoreUniform(location, 1, v, 4, true);
}

void PS4_SYSV_ABI glUniform1fv(s32 location, s32 count, const float* value) {
    StoreUniform(location, count, value, 1);
}

void PS4_SYSV_ABI glUniform2fv(s32 location, s32 count, const float* value) {
    StoreUniform(location, count, value, 2);
}

void PS4_SYSV_ABI glUniform3fv(s32 location, s32 count, const float* value) {
    StoreUniform(location, count, value, 3);
}

void PS4_SYSV_ABI glUniform4fv(s32 location, s32 count, const float* value) {
    StoreUniform(location, count, value, 4);
}

void PS4_SYSV_ABI glUniform1iv(s32 location, s32 count, const s32* value) {
    StoreUniform(location, count, value, 1);
}

void PS4_SYSV_ABI glUniform2iv(s32 location, s32 count, const s32* value) {
    StoreUniform(location, count, value, 2);
}

void PS4_SYSV_ABI glUniform3iv(s32 location, s32 count, const s32* value) {
    StoreUniform(location, count, value, 3);
}

void PS4_SYSV_ABI glUniform4iv(s32 location, s32 count, const s32* value) {
    StoreUniform(location, count, value, 4);
}

void PS4_SYSV_ABI glUniform1uiv(s32 location, s32 count, const u32* value) {
    StoreUniform(location, count, value, 1, true);
}

void PS4_SYSV_ABI glUniform2uiv(s32 location, s32 count, const u32* value) {
    StoreUniform(location, count, value, 2, true);
}

void PS4_SYSV_ABI glUniform3uiv(s32 location, s32 count, const u32* value) {
    StoreUniform(location, count, value, 3, true);
}

void PS4_SYSV_ABI glUniform4uiv(s32 location, s32 count, const u32* value) {
    StoreUniform(location, count, value, 4, true);
}

void PS4_SYSV_ABI glUniformMatrix2fv(s32 location, s32 count, u8 transpose, const float* value) {
    StoreUniformMatrix(location, count, transpose, value, 4);
}

void PS4_SYSV_ABI glUniformMatrix3fv(s32 location, s32 count, u8 transpose, const float* value) {
    StoreUniformMatrix(location, count, transpose, value, 9);
}

void PS4_SYSV_ABI glUniformMatrix4fv(s32 location, s32 count, u8 transpose, const float* value) {
    StoreUniformMatrix(location, count, transpose, value, 16);
}

void PS4_SYSV_ABI glUniformMatrix2x3fv(s32 location, s32 count, u8 transpose, const float* value) {
    StoreUniformMatrix(location, count, transpose, value, 6, true);
}

void PS4_SYSV_ABI glUniformMatrix3x2fv(s32 location, s32 count, u8 transpose, const float* value) {
    StoreUniformMatrix(location, count, transpose, value, 6, true);
}

void PS4_SYSV_ABI glUniformMatrix2x4fv(s32 location, s32 count, u8 transpose, const float* value) {
    StoreUniformMatrix(location, count, transpose, value, 8, true);
}

void PS4_SYSV_ABI glUniformMatrix4x2fv(s32 location, s32 count, u8 transpose, const float* value) {
    StoreUniformMatrix(location, count, transpose, value, 8, true);
}

void PS4_SYSV_ABI glUniformMatrix3x4fv(s32 location, s32 count, u8 transpose, const float* value) {
    StoreUniformMatrix(location, count, transpose, value, 12, true);
}

void PS4_SYSV_ABI glUniformMatrix4x3fv(s32 location, s32 count, u8 transpose, const float* value) {
    StoreUniformMatrix(location, count, transpose, value, 12, true);
}

void PS4_SYSV_ABI glGetUniformfv(u32 program, s32 location, float* params) {
    GetUniform(program, location, params);
}

void PS4_SYSV_ABI glGetUniformiv(u32 program, s32 location, s32* params) {
    GetUniform(program, location, params);
}

void PS4_SYSV_ABI glGetUniformuiv(u32 program, s32 location, u32* params) {
    GetUniform(program, location, params);
}

s32 PS4_SYSV_ABI glGetUniformLocation(u32 program, const char* name) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return -1;
    }
    const GlProgram* prog = FindProgram(*gl, program);
    if (prog == nullptr) {
        return -1;
    }
    if (!prog->linked) {
        SetGlError(*gl, GL_INVALID_OPERATION);
    }
    // TODO: Needs program reflection from the shader binary.
    return -1;
}

s32 PS4_SYSV_ABI glGetFragDataLocation(u32 program, const char* name) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return -1;
    }
    const GlProgram* prog = FindProgram(*gl, program);
    if (prog != nullptr && !prog->linked) {
        SetGlError(*gl, GL_INVALID_OPERATION);
    }
    return -1;
}

void PS4_SYSV_ABI glBindAttribLocation(u32 program, u32 index, const char* name) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    GlProgram* prog = FindProgram(*gl, program);
    if (prog == nullptr) {
        return;
    }
    if (index >= MAX_VERTEX_ATTRIBS) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    if (std::strncmp(name, "gl_", 3) == 0) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    prog->attrib_bindings[name] = index;
}

s32 PS4_SYSV_ABI glGetAttribLocation(u32 program, const char* name) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return -1;
    }
    const GlProgram* prog = FindProgram(*gl, program);
    if (prog == nullptr) {
        return -1;
    }
    if (!prog->linked) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return -1;
    }
    const auto it = prog->attrib_bindings.find(name);
    return it != prog->attrib_bindings.end() ? static_cast<s32>(it->second) : -1;
}

// Programs have no active attributes or uniforms until shader reflection exists.
void PS4_SYSV_ABI glGetActiveAttrib(u32 program, u32 index, s32 buf_size, s32* length, s32* size,
                                    u32* type, char* name) {
    GlContext* gl = RequireContext();
    if (gl != nullptr && FindProgram(*gl, program) != nullptr) {
        SetGlError(*gl, GL_INVALID_VALUE);
    }
}

void PS4_SYSV_ABI glGetActiveUniform(u32 program, u32 index, s32 buf_size, s32* length, s32* size,
                                     u32* type, char* name) {
    GlContext* gl = RequireContext();
    if (gl != nullptr && FindProgram(*gl, program) != nullptr) {
        SetGlError(*gl, GL_INVALID_VALUE);
    }
}

void PS4_SYSV_ABI glGetUniformIndices(u32 program, s32 count, const char* const* names,
                                      u32* indices) {
    GlContext* gl = RequireContext();
    if (gl == nullptr || FindProgram(*gl, program) == nullptr) {
        return;
    }
    if (count < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    std::fill_n(indices, count, GL_INVALID_INDEX);
}

void PS4_SYSV_ABI glGetActiveUniformsiv(u32 program, s32 count, const u32* indices, u32 pname,
                                        s32* params) {
    GlContext* gl = RequireContext();
    if (gl != nullptr && FindProgram(*gl, program) != nullptr) {
        SetGlError(*gl, count < 0 ? GL_INVALID_VALUE : GL_INVALID_VALUE);
    }
}

u32 PS4_SYSV_ABI glGetUniformBlockIndex(u32 program, const char* name) {
    GlContext* gl = RequireContext();
    if (gl != nullptr && FindProgram(*gl, program) != nullptr) {
        return GL_INVALID_INDEX;
    }
    return GL_INVALID_INDEX;
}

void PS4_SYSV_ABI glGetActiveUniformBlockiv(u32 program, u32 index, u32 pname, s32* params) {
    GlContext* gl = RequireContext();
    if (gl != nullptr && FindProgram(*gl, program) != nullptr) {
        SetGlError(*gl, GL_INVALID_VALUE);
    }
}

void PS4_SYSV_ABI glGetActiveUniformBlockName(u32 program, u32 index, s32 buf_size, s32* length,
                                              char* name) {
    GlContext* gl = RequireContext();
    if (gl != nullptr && FindProgram(*gl, program) != nullptr) {
        SetGlError(*gl, GL_INVALID_VALUE);
    }
}

void PS4_SYSV_ABI glUniformBlockBinding(u32 program, u32 index, u32 binding) {
    GlContext* gl = RequireContext();
    if (gl != nullptr && FindProgram(*gl, program) != nullptr) {
        SetGlError(*gl, GL_INVALID_VALUE);
    }
}

void PS4_SYSV_ABI glValidateProgram(u32 program) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (GlProgram* prog = FindProgram(*gl, program)) {
        prog->validated = prog->linked;
    }
}

void PS4_SYSV_ABI glGetProgramBinary(u32 program, s32 buf_size, s32* length, u32* binary_format,
                                     void* binary) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const GlProgram* prog = FindProgram(*gl, program);
    if (prog == nullptr) {
        return;
    }
    if (!prog->linked || buf_size < 0) {
        SetGlError(*gl, prog->linked ? GL_INVALID_VALUE : GL_INVALID_OPERATION);
        return;
    }
    // TODO: Programs are not backed by a real binary yet.
    if (length != nullptr) {
        *length = 0;
    }
    if (binary_format != nullptr) {
        *binary_format = 0;
    }
}

void PS4_SYSV_ABI glProgramBinary(u32 program, u32 binary_format, const void* binary, s32 length) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    GlProgram* prog = FindProgram(*gl, program);
    if (prog == nullptr) {
        return;
    }
    // TODO: No program binary format is supported yet, so loading one fails to link.
    prog->linked = false;
    prog->info_log = "Program binary format is not supported\n";
}

void PS4_SYSV_ABI glProgramParameteri(u32 program, u32 pname, s32 value) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    GlProgram* prog = FindProgram(*gl, program);
    if (prog == nullptr) {
        return;
    }
    if (pname != 0x8257) { // GL_PROGRAM_BINARY_RETRIEVABLE_HINT
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (value != 0 && value != 1) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    prog->binary_retrievable_hint = value != 0;
}

void PS4_SYSV_ABI glGetShaderPrecisionFormat(u32 shader_type, u32 precision_type, s32* range,
                                             s32* precision) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if ((shader_type != GL_VERTEX_SHADER && shader_type != GL_FRAGMENT_SHADER) ||
        precision_type < 0x8DF0 || precision_type > 0x8DF5) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    // Values reported by Piglet: highp/mediump/lowp float and int.
    const bool is_float = precision_type <= 0x8DF2;
    range[0] = is_float ? 127 : 30;
    range[1] = is_float ? 127 : 21;
    *precision = is_float ? 23 : 0;
}

void PS4_SYSV_ABI glTexParameteri(u32 target, u32 pname, s32 param) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsTexture2DTarget(target) && !IsTexture3DTarget(*gl, target)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (!IsTextureParam(*gl, pname)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    SetTextureParam(*gl, *BoundTexture(*gl, target), pname, param);
}

void PS4_SYSV_ABI glTexParameterf(u32 target, u32 pname, float param) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsTexture2DTarget(target) && !IsTexture3DTarget(*gl, target)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (!IsTextureParam(*gl, pname)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    SetTextureParam(*gl, *BoundTexture(*gl, target), pname, param);
}

void PS4_SYSV_ABI glTexParameteriv(u32 target, u32 pname, const s32* params) {
    glTexParameteri(target, pname, params[0]);
}

void PS4_SYSV_ABI glTexParameterfv(u32 target, u32 pname, const float* params) {
    glTexParameterf(target, pname, params[0]);
}

void PS4_SYSV_ABI glGetTexParameteriv(u32 target, u32 pname, s32* params) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if ((!IsTexture2DTarget(target) && !IsTexture3DTarget(*gl, target)) ||
        !IsTextureParam(*gl, pname)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    *params = ToInteger(GetTextureParam(*BoundTexture(*gl, target), pname));
}

void PS4_SYSV_ABI glGetTexParameterfv(u32 target, u32 pname, float* params) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if ((!IsTexture2DTarget(target) && !IsTexture3DTarget(*gl, target)) ||
        !IsTextureParam(*gl, pname)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    *params = static_cast<float>(GetTextureParam(*BoundTexture(*gl, target), pname));
}

void PS4_SYSV_ABI glTexImage2D(u32 target, s32 level, s32 internal_format, s32 width, s32 height,
                               s32 border, u32 format, u32 type, const void* pixels) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != GL_TEXTURE_2D && !IsCubeFace(target)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (!IsTextureFormat(*gl, format) || !IsPixelType(type)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (border != 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    // TODO: Store the pixel data once there is a rendering backend.
    SetTextureLevel(*gl, target, level, width, height, 1, static_cast<u32>(internal_format), false);
}

void PS4_SYSV_ABI glTexSubImage2D(u32 target, s32 level, s32 xoffset, s32 yoffset, s32 width,
                                  s32 height, u32 format, u32 type, const void* pixels) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != GL_TEXTURE_2D && !IsCubeFace(target)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (!IsTextureFormat(*gl, format) || !IsPixelType(type)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    CheckSubImage(*gl, target, level, xoffset, yoffset, 0, width, height, 1);
}

void PS4_SYSV_ABI glCopyTexImage2D(u32 target, s32 level, u32 internal_format, s32 x, s32 y,
                                   s32 width, s32 height, s32 border) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != GL_TEXTURE_2D && !IsCubeFace(target)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (border != 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    if (!CheckDrawFramebuffer(*gl)) {
        return;
    }
    SetTextureLevel(*gl, target, level, width, height, 1, internal_format, false);
}

void PS4_SYSV_ABI glCopyTexSubImage2D(u32 target, s32 level, s32 xoffset, s32 yoffset, s32 x, s32 y,
                                      s32 width, s32 height) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != GL_TEXTURE_2D && !IsCubeFace(target)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (CheckDrawFramebuffer(*gl)) {
        CheckSubImage(*gl, target, level, xoffset, yoffset, 0, width, height, 1);
    }
}

void PS4_SYSV_ABI glCompressedTexImage2D(u32 target, s32 level, u32 internal_format, s32 width,
                                         s32 height, s32 border, s32 image_size, const void* data) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != GL_TEXTURE_2D && !IsCubeFace(target)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (!IsCompressedFormat(internal_format)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (border != 0 || image_size < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    SetTextureLevel(*gl, target, level, width, height, 1, internal_format, true);
}

void PS4_SYSV_ABI glCompressedTexSubImage2D(u32 target, s32 level, s32 xoffset, s32 yoffset,
                                            s32 width, s32 height, u32 format, s32 image_size,
                                            const void* data) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != GL_TEXTURE_2D && !IsCubeFace(target)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (!IsCompressedFormat(format)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (image_size < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    CheckSubImage(*gl, target, level, xoffset, yoffset, 0, width, height, 1);
}

void PS4_SYSV_ABI glTexImage3D(u32 target, s32 level, s32 internal_format, s32 width, s32 height,
                               s32 depth, s32 border, u32 format, u32 type, const void* pixels) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsTexture3DTarget(*gl, target) || !IsTextureFormat(*gl, format) || !IsPixelType(type)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (border != 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    SetTextureLevel(*gl, target, level, width, height, depth, static_cast<u32>(internal_format),
                    false);
}

void PS4_SYSV_ABI glTexSubImage3D(u32 target, s32 level, s32 xoffset, s32 yoffset, s32 zoffset,
                                  s32 width, s32 height, s32 depth, u32 format, u32 type,
                                  const void* pixels) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsTexture3DTarget(*gl, target) || !IsTextureFormat(*gl, format) || !IsPixelType(type)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    CheckSubImage(*gl, target, level, xoffset, yoffset, zoffset, width, height, depth);
}

void PS4_SYSV_ABI glCopyTexSubImage3D(u32 target, s32 level, s32 xoffset, s32 yoffset, s32 zoffset,
                                      s32 x, s32 y, s32 width, s32 height) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsTexture3DTarget(*gl, target)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (CheckDrawFramebuffer(*gl)) {
        CheckSubImage(*gl, target, level, xoffset, yoffset, zoffset, width, height, 1);
    }
}

void PS4_SYSV_ABI glCompressedTexImage3D(u32 target, s32 level, u32 internal_format, s32 width,
                                         s32 height, s32 depth, s32 border, s32 image_size,
                                         const void* data) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsTexture3DTarget(*gl, target) || !IsCompressedFormat(internal_format)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (border != 0 || image_size < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    SetTextureLevel(*gl, target, level, width, height, depth, internal_format, true);
}

void PS4_SYSV_ABI glCompressedTexSubImage3D(u32 target, s32 level, s32 xoffset, s32 yoffset,
                                            s32 zoffset, s32 width, s32 height, s32 depth,
                                            u32 format, s32 image_size, const void* data) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsTexture3DTarget(*gl, target) || !IsCompressedFormat(format)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (image_size < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    CheckSubImage(*gl, target, level, xoffset, yoffset, zoffset, width, height, depth);
}

// Shared by glTexStorage2D/3D and the EXT variants.
static void TexStorage(GlContext& gl, u32 target, s32 levels, u32 internal_format, s32 width,
                       s32 height, s32 depth) {
    if (levels < 1 || width < 1 || height < 1 || depth < 1) {
        SetGlError(gl, GL_INVALID_VALUE);
        return;
    }
    if (levels > MaxLevels(std::max({width, height, depth}))) {
        SetGlError(gl, GL_INVALID_OPERATION);
        return;
    }
    GlTexture* texture = BoundTexture(gl, BindingTarget(target));
    if (texture->immutable) {
        SetGlError(gl, GL_INVALID_OPERATION);
        return;
    }
    for (s32 level = 0; level < levels; level++) {
        if (!SetTextureLevel(gl, target, level, std::max(width >> level, 1),
                             std::max(height >> level, 1), std::max(depth >> level, 1),
                             internal_format, false)) {
            return;
        }
    }
    texture->immutable = true;
}

void PS4_SYSV_ABI glTexStorage2D(u32 target, s32 levels, u32 internal_format, s32 width,
                                 s32 height) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != GL_TEXTURE_2D && target != GL_TEXTURE_CUBE_MAP) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    TexStorage(*gl, target, levels, internal_format, width, height, 1);
}

void PS4_SYSV_ABI glTexStorage2DEXT(u32 target, s32 levels, u32 internal_format, s32 width,
                                    s32 height) {
    glTexStorage2D(target, levels, internal_format, width, height);
}

void PS4_SYSV_ABI glTextureStorage2DEXT(u32 texture, u32 target, s32 levels, u32 internal_format,
                                        s32 width, s32 height) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != GL_TEXTURE_2D && target != GL_TEXTURE_CUBE_MAP) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (texture == 0 || !gl->textures.IsObject(texture)) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    // Direct state access: the texture is not the one bound to the active unit.
    auto& binding = gl->texture_bindings[gl->active_texture - GL_TEXTURE0][target];
    const u32 previous = std::exchange(binding, texture);
    TexStorage(*gl, target, levels, internal_format, width, height, 1);
    binding = previous;
}

void PS4_SYSV_ABI glTexStorage3D(u32 target, s32 levels, u32 internal_format, s32 width, s32 height,
                                 s32 depth) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsTexture3DTarget(*gl, target)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    TexStorage(*gl, target, levels, internal_format, width, height, depth);
}

void PS4_SYSV_ABI glGenerateMipmap(u32 target) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsTexture2DTarget(target) && !IsTexture3DTarget(*gl, target)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    const GlTexture* texture = BoundTexture(*gl, target);
    if (!std::ranges::any_of(texture->levels,
                             [](const auto& level) { return level.first.second == 0; })) {
        SetGlError(*gl, GL_INVALID_OPERATION);
    }
}

void PS4_SYSV_ABI glFramebufferRenderbuffer(u32 target, u32 attachment, u32 renderbuffer_target,
                                            u32 renderbuffer) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const bool color = attachment >= GL_COLOR_ATTACHMENT0 &&
                       attachment < GL_COLOR_ATTACHMENT0 + MAX_COLOR_ATTACHMENTS;
    if ((target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER &&
         target != GL_DRAW_FRAMEBUFFER) ||
        renderbuffer_target != GL_RENDERBUFFER ||
        !(color || attachment == GL_DEPTH_ATTACHMENT || attachment == GL_STENCIL_ATTACHMENT ||
          (gl->version >= 3 && attachment == GL_DEPTH_STENCIL_ATTACHMENT))) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    const u32 name = target == GL_READ_FRAMEBUFFER ? gl->read_framebuffer : gl->draw_framebuffer;
    GlFramebuffer* framebuffer = gl->framebuffers.Find(name);
    if (framebuffer == nullptr ||
        (renderbuffer != 0 && !gl->renderbuffers.IsObject(renderbuffer))) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    framebuffer->attachments[attachment] = {renderbuffer != 0 ? GL_RENDERBUFFER : 0u, renderbuffer,
                                            0, 0, 0};
}

void PS4_SYSV_ABI glFramebufferTexture2D(u32 target, u32 attachment, u32 texture_target,
                                         u32 texture, s32 level) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const bool color = attachment >= GL_COLOR_ATTACHMENT0 &&
                       attachment < GL_COLOR_ATTACHMENT0 + MAX_COLOR_ATTACHMENTS;
    if ((target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER &&
         target != GL_DRAW_FRAMEBUFFER) ||
        !(color || attachment == GL_DEPTH_ATTACHMENT || attachment == GL_STENCIL_ATTACHMENT ||
          (gl->version >= 3 && attachment == GL_DEPTH_STENCIL_ATTACHMENT)) ||
        (texture != 0 && texture_target != GL_TEXTURE_2D && !IsCubeFace(texture_target))) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (level < 0 || (gl->version < 3 && level != 0)) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    const u32 name = target == GL_READ_FRAMEBUFFER ? gl->read_framebuffer : gl->draw_framebuffer;
    GlFramebuffer* framebuffer = gl->framebuffers.Find(name);
    if (framebuffer == nullptr || (texture != 0 && !gl->textures.IsObject(texture))) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    framebuffer->attachments[attachment] = {texture != 0 ? 0x1702u : 0u, texture, level, 0,
                                            texture_target};
}

void PS4_SYSV_ABI glFramebufferTextureLayer(u32 target, u32 attachment, u32 texture, s32 level,
                                            s32 layer) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const bool color = attachment >= GL_COLOR_ATTACHMENT0 &&
                       attachment < GL_COLOR_ATTACHMENT0 + MAX_COLOR_ATTACHMENTS;
    if ((target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER &&
         target != GL_DRAW_FRAMEBUFFER) ||
        !(color || attachment == GL_DEPTH_ATTACHMENT || attachment == GL_STENCIL_ATTACHMENT ||
          attachment == GL_DEPTH_STENCIL_ATTACHMENT)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (level < 0 || layer < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    const u32 name = target == GL_READ_FRAMEBUFFER ? gl->read_framebuffer : gl->draw_framebuffer;
    GlFramebuffer* framebuffer = gl->framebuffers.Find(name);
    GlTexture* object = gl->textures.Find(texture);
    if (framebuffer == nullptr ||
        (texture != 0 && (object == nullptr || !IsTexture3DTarget(*gl, object->target)))) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    framebuffer->attachments[attachment] = {texture != 0 ? 0x1702u : 0u, texture, level, layer,
                                            object != nullptr ? object->target : 0u};
}

u32 PS4_SYSV_ABI glCheckFramebufferStatus(u32 target) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return 0;
    }
    if (target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER &&
        target != GL_DRAW_FRAMEBUFFER) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return 0;
    }
    return FramebufferStatus(*gl, target == GL_READ_FRAMEBUFFER ? gl->read_framebuffer
                                                                : gl->draw_framebuffer);
}

void PS4_SYSV_ABI glGetFramebufferAttachmentParameteriv(u32 target, u32 attachment, u32 pname,
                                                        s32* params) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER &&
        target != GL_DRAW_FRAMEBUFFER) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    const u32 name = target == GL_READ_FRAMEBUFFER ? gl->read_framebuffer : gl->draw_framebuffer;
    if (name == 0) {
        // Default framebuffer: only the object type is meaningful.
        if (gl->version < 3 || pname != 0x8CD0) {
            SetGlError(*gl, GL_INVALID_ENUM);
            return;
        }
        *params = 0x8218; // GL_FRAMEBUFFER_DEFAULT
        return;
    }
    const GlFramebuffer* framebuffer = gl->framebuffers.Find(name);
    if (framebuffer == nullptr) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    const auto it = framebuffer->attachments.find(attachment);
    const GlFramebufferAttachment none;
    const GlFramebufferAttachment& value = it != framebuffer->attachments.end() ? it->second : none;
    switch (pname) {
    case 0x8CD0: // GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE
        *params = static_cast<s32>(value.type);
        break;
    case 0x8CD1: // GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME
        *params = static_cast<s32>(value.name);
        break;
    case 0x8CD2: // GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL
        *params = value.level;
        break;
    case 0x8CD3: // GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE
        *params = IsCubeFace(value.texture_target) ? static_cast<s32>(value.texture_target) : 0;
        break;
    case 0x8CD4: // GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER
        *params = value.layer;
        break;
    default:
        SetGlError(*gl, GL_INVALID_ENUM);
        break;
    }
}

void PS4_SYSV_ABI glGetRenderbufferParameteriv(u32 target, u32 pname, s32* params) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != GL_RENDERBUFFER) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    const GlRenderbuffer* renderbuffer = gl->renderbuffers.Find(gl->renderbuffer);
    if (renderbuffer == nullptr) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    switch (pname) {
    case 0x8D42: // GL_RENDERBUFFER_WIDTH
        *params = renderbuffer->width;
        break;
    case 0x8D43: // GL_RENDERBUFFER_HEIGHT
        *params = renderbuffer->height;
        break;
    case 0x8D44: // GL_RENDERBUFFER_INTERNAL_FORMAT
        *params = static_cast<s32>(renderbuffer->internal_format);
        break;
    case 0x8CAB: // GL_RENDERBUFFER_SAMPLES
        *params = renderbuffer->samples;
        break;
    default:
        // TODO: Component sizes depend on the internal format table.
        SetGlError(*gl, GL_INVALID_ENUM);
        break;
    }
}

void PS4_SYSV_ABI glRenderbufferStorageMultisample(u32 target, s32 samples, u32 internal_format,
                                                   s32 width, s32 height) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != GL_RENDERBUFFER) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (samples < 0 || samples > 8) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    glRenderbufferStorage(target, internal_format, width, height);
    if (GlRenderbuffer* renderbuffer = gl->renderbuffers.Find(gl->renderbuffer)) {
        renderbuffer->samples = samples;
    }
}

void PS4_SYSV_ABI glGetInternalformativ(u32 target, u32 internal_format, u32 pname, s32 buf_size,
                                        s32* params) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != GL_RENDERBUFFER || (pname != 0x9380 && pname != 0x80A9)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (buf_size < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    // TODO: Only the formats checked against Piglet (RGBA8 and DEPTH_COMPONENT16, which report 4
    // sample counts) are handled. Other formats report no multisampling support.
    const bool multisampled = internal_format == 0x8058 || internal_format == 0x81A5;
    constexpr std::array<s32, 4> SAMPLE_COUNTS = {8, 4, 2, 1};
    if (pname == 0x9380) {
        if (buf_size > 0) {
            params[0] = multisampled ? static_cast<s32>(SAMPLE_COUNTS.size()) : 0;
        }
    } else if (multisampled) {
        const s32 count = std::min(buf_size, static_cast<s32>(SAMPLE_COUNTS.size()));
        std::copy_n(SAMPLE_COUNTS.begin(), count, params);
    }
}

void PS4_SYSV_ABI glReadBuffer(u32 src) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const bool color =
        src >= GL_COLOR_ATTACHMENT0 && src < GL_COLOR_ATTACHMENT0 + MAX_COLOR_ATTACHMENTS;
    if (gl->read_framebuffer == 0) {
        if (src != 0x0405 && src != 0) { // GL_BACK, GL_NONE
            SetGlError(*gl, GL_INVALID_ENUM);
            return;
        }
        gl->default_read_buffer = src;
        return;
    }
    if (src != 0 && !color) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (GlFramebuffer* framebuffer = gl->framebuffers.Find(gl->read_framebuffer)) {
        framebuffer->read_buffer = src;
    }
}

void PS4_SYSV_ABI glDrawBuffers(s32 n, const u32* bufs) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (n < 0 || n > MAX_DRAW_BUFFERS) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    std::vector<u32> buffers(bufs, bufs + n);
    for (const u32 buffer : buffers) {
        const bool color =
            buffer >= GL_COLOR_ATTACHMENT0 && buffer < GL_COLOR_ATTACHMENT0 + MAX_COLOR_ATTACHMENTS;
        if (buffer != 0 && !color && !(gl->draw_framebuffer == 0 && buffer == 0x0405)) {
            SetGlError(*gl, GL_INVALID_ENUM);
            return;
        }
    }
    if (gl->draw_framebuffer == 0) {
        gl->default_draw_buffers = std::move(buffers);
    } else if (GlFramebuffer* framebuffer = gl->framebuffers.Find(gl->draw_framebuffer)) {
        framebuffer->draw_buffers = std::move(buffers);
    }
}

void PS4_SYSV_ABI glBlitFramebuffer(s32 src_x0, s32 src_y0, s32 src_x1, s32 src_y1, s32 dst_x0,
                                    s32 dst_y0, s32 dst_x1, s32 dst_y1, u32 mask, u32 filter) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if ((mask & ~0x4500u) != 0 || (filter != 0x2600 && filter != 0x2601)) {
        SetGlError(*gl, (mask & ~0x4500u) != 0 ? GL_INVALID_VALUE : GL_INVALID_ENUM);
        return;
    }
    // TODO: Copy the pixels once there is a rendering backend.
    CheckDrawFramebuffer(*gl);
}

void PS4_SYSV_ABI glInvalidateFramebuffer(u32 target, s32 num_attachments, const u32* attachments) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER &&
        target != GL_DRAW_FRAMEBUFFER) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (num_attachments < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
    }
}

void PS4_SYSV_ABI glInvalidateSubFramebuffer(u32 target, s32 num_attachments,
                                             const u32* attachments, s32 x, s32 y, s32 width,
                                             s32 height) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER &&
        target != GL_DRAW_FRAMEBUFFER) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (num_attachments < 0 || width < 0 || height < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
    }
}

void PS4_SYSV_ABI glClear(u32 mask) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if ((mask & ~0x4500u) != 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    // TODO: Clear the framebuffer once there is a rendering backend.
    CheckDrawFramebuffer(*gl);
}

// Validates a glClearBuffer* call.
static bool CheckClearBuffer(GlContext& gl, u32 buffer, s32 drawbuffer, bool depth_stencil) {
    const bool is_color = buffer == 0x1800;
    const bool is_depth_stencil = buffer == 0x1801 || buffer == 0x1802 || buffer == 0x84F9;
    if (!is_color && !is_depth_stencil) {
        SetGlError(gl, GL_INVALID_ENUM);
        return false;
    }
    if ((is_color && depth_stencil) || (!is_color && !depth_stencil && buffer != 0x1801) ||
        drawbuffer < 0 || (drawbuffer >= MAX_DRAW_BUFFERS) || (!is_color && drawbuffer != 0)) {
        SetGlError(gl, (is_color && depth_stencil) ? GL_INVALID_ENUM : GL_INVALID_VALUE);
        return false;
    }
    return CheckDrawFramebuffer(gl);
}

void PS4_SYSV_ABI glClearBufferiv(u32 buffer, s32 drawbuffer, const s32* value) {
    if (GlContext* gl = RequireContext()) {
        if (buffer == 0x1801 || buffer == 0x84F9) {
            SetGlError(*gl, GL_INVALID_ENUM);
            return;
        }
        CheckClearBuffer(*gl, buffer, drawbuffer, false);
    }
}

void PS4_SYSV_ABI glClearBufferuiv(u32 buffer, s32 drawbuffer, const u32* value) {
    if (GlContext* gl = RequireContext()) {
        if (buffer != 0x1800) {
            SetGlError(*gl, GL_INVALID_ENUM);
            return;
        }
        CheckClearBuffer(*gl, buffer, drawbuffer, false);
    }
}

void PS4_SYSV_ABI glClearBufferfv(u32 buffer, s32 drawbuffer, const float* value) {
    if (GlContext* gl = RequireContext()) {
        if (buffer == 0x1802 || buffer == 0x84F9) {
            SetGlError(*gl, GL_INVALID_ENUM);
            return;
        }
        CheckClearBuffer(*gl, buffer, drawbuffer, false);
    }
}

void PS4_SYSV_ABI glClearBufferfi(u32 buffer, s32 drawbuffer, float depth, s32 stencil) {
    if (GlContext* gl = RequireContext()) {
        if (buffer != 0x84F9) {
            SetGlError(*gl, GL_INVALID_ENUM);
            return;
        }
        CheckClearBuffer(*gl, buffer, drawbuffer, true);
    }
}

void PS4_SYSV_ABI glDrawArrays(u32 mode, s32 first, s32 count) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (first < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    // TODO: Draw once there is a rendering backend.
    CheckDraw(*gl, mode, count);
}

void PS4_SYSV_ABI glDrawElements(u32 mode, s32 count, u32 type, const void* indices) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsIndexType(type)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    CheckDraw(*gl, mode, count);
}

void PS4_SYSV_ABI glDrawArraysInstanced(u32 mode, s32 first, s32 count, s32 instance_count) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (first < 0 || instance_count < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    CheckDraw(*gl, mode, count);
}

void PS4_SYSV_ABI glDrawArraysInstancedEXT(u32 mode, s32 first, s32 count, s32 instance_count) {
    glDrawArraysInstanced(mode, first, count, instance_count);
}

void PS4_SYSV_ABI glDrawElementsInstanced(u32 mode, s32 count, u32 type, const void* indices,
                                          s32 instance_count) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsIndexType(type)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (instance_count < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    CheckDraw(*gl, mode, count);
}

void PS4_SYSV_ABI glDrawElementsInstancedEXT(u32 mode, s32 count, u32 type, const void* indices,
                                             s32 instance_count) {
    glDrawElementsInstanced(mode, count, type, indices, instance_count);
}

void PS4_SYSV_ABI glDrawRangeElements(u32 mode, u32 start, u32 end, s32 count, u32 type,
                                      const void* indices) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsIndexType(type)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (end < start) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    CheckDraw(*gl, mode, count);
}

void PS4_SYSV_ABI glReadPixels(s32 x, s32 y, s32 width, s32 height, u32 format, u32 type,
                               void* pixels) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (width < 0 || height < 0) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    if (!IsTextureFormat(*gl, format) || !IsPixelType(type)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    const s32 pixel_size = PixelSize(format, type);
    if (pixel_size == 0) {
        // TODO: Only RGBA/BGRA/RGB with unsigned bytes can be read back.
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    if (FramebufferStatus(*gl, gl->read_framebuffer) != GL_FRAMEBUFFER_COMPLETE) {
        SetGlError(*gl, GL_INVALID_FRAMEBUFFER_OPERATION);
        return;
    }
    if (gl->other_buffer_bindings[0x88EB] != 0) {
        // Reading into a pixel pack buffer is not supported yet.
        return;
    }
    // Nothing is rendered, so the framebuffer reads back as transparent black.
    if (pixels == nullptr) {
        return;
    }
    const s64 alignment = gl->pixel_store.at(0x0D05);
    const s64 row_size =
        (static_cast<s64>(width) * pixel_size + alignment - 1) / alignment * alignment;
    std::memset(pixels, 0, static_cast<size_t>(row_size * height));
}

void* PS4_SYSV_ABI glMapBufferRange(u32 target, s64 offset, s64 length, u32 access) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return nullptr;
    }
    u32* binding = BufferBinding(*gl, target);
    if (binding == nullptr) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return nullptr;
    }
    GlBuffer* buffer = gl->buffers.Find(*binding);
    if (buffer == nullptr) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return nullptr;
    }
    if (offset < 0 || length < 0 || offset + length > static_cast<s64>(buffer->data.size())) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return nullptr;
    }
    const bool flush_explicit = (access & 0x10) != 0;
    if ((access & ~0x3Fu) != 0 || (access & 3) == 0 || (flush_explicit && (access & 2) == 0) ||
        ((access & 1) != 0 && (access & 0x1C) != 0)) {
        SetGlError(*gl, (access & ~0x3Fu) != 0 ? GL_INVALID_VALUE : GL_INVALID_OPERATION);
        return nullptr;
    }
    if (buffer->mapped || length == 0) {
        SetGlError(*gl, buffer->mapped ? GL_INVALID_OPERATION : GL_INVALID_VALUE);
        return nullptr;
    }
    buffer->mapped = true;
    buffer->map_offset = offset;
    buffer->map_length = length;
    buffer->map_access = access;
    return buffer->data.data() + offset;
}

void PS4_SYSV_ABI glFlushMappedBufferRange(u32 target, s64 offset, s64 length) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    u32* binding = BufferBinding(*gl, target);
    if (binding == nullptr) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    const GlBuffer* buffer = gl->buffers.Find(*binding);
    if (buffer == nullptr || !buffer->mapped || (buffer->map_access & 0x10) == 0) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    if (offset < 0 || length < 0 || offset + length > buffer->map_length) {
        SetGlError(*gl, GL_INVALID_VALUE);
    }
}

u8 PS4_SYSV_ABI glUnmapBuffer(u32 target) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return 0;
    }
    u32* binding = BufferBinding(*gl, target);
    if (binding == nullptr) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return 0;
    }
    GlBuffer* buffer = gl->buffers.Find(*binding);
    if (buffer == nullptr || !buffer->mapped) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return 0;
    }
    buffer->mapped = false;
    return 1;
}

void PS4_SYSV_ABI glGetBufferPointerv(u32 target, u32 pname, void** params) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    u32* binding = BufferBinding(*gl, target);
    if (binding == nullptr || pname != 0x88BD) { // GL_BUFFER_MAP_POINTER
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    GlBuffer* buffer = gl->buffers.Find(*binding);
    if (buffer == nullptr) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    *params = buffer->mapped ? buffer->data.data() + buffer->map_offset : nullptr;
}

void PS4_SYSV_ABI glGetBufferParameteri64v(u32 target, u32 pname, s64* params) {
    s32 value = 0;
    glGetBufferParameteriv(target, pname, &value);
    *params = value;
}

void PS4_SYSV_ABI glCopyBufferSubData(u32 read_target, u32 write_target, s64 read_offset,
                                      s64 write_offset, s64 size) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    u32* read_binding = BufferBinding(*gl, read_target);
    u32* write_binding = BufferBinding(*gl, write_target);
    if (read_binding == nullptr || write_binding == nullptr) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    GlBuffer* source = gl->buffers.Find(*read_binding);
    GlBuffer* destination = gl->buffers.Find(*write_binding);
    if (source == nullptr || destination == nullptr || source->mapped || destination->mapped) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    if (read_offset < 0 || write_offset < 0 || size < 0 ||
        read_offset + size > static_cast<s64>(source->data.size()) ||
        write_offset + size > static_cast<s64>(destination->data.size())) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    if (source == destination && read_offset < write_offset + size &&
        write_offset < read_offset + size) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    std::memmove(destination->data.data() + write_offset, source->data.data() + read_offset,
                 static_cast<size_t>(size));
}

// Indexed buffer binding shared by glBindBufferBase and glBindBufferRange.
static void BindIndexedBuffer(GlContext& gl, u32 target, u32 index, u32 buffer, s64 offset,
                              s64 size) {
    GlIndexedBinding* binding = nullptr;
    if (target == 0x8A11) { // GL_UNIFORM_BUFFER
        if (index < MAX_UNIFORM_BUFFER_BINDINGS) {
            binding = &gl.uniform_buffer_bindings[index];
        }
    } else if (target == 0x8C8E) { // GL_TRANSFORM_FEEDBACK_BUFFER
        if (index < MAX_TRANSFORM_FEEDBACK_ATTRIBS) {
            binding = &gl.transform_feedback_bindings[index];
        }
    } else {
        SetGlError(gl, GL_INVALID_ENUM);
        return;
    }
    if (binding == nullptr) {
        SetGlError(gl, GL_INVALID_VALUE);
        return;
    }
    if (buffer != 0) {
        gl.buffers.Bind(buffer);
    }
    *binding = {buffer, offset, size};
    gl.other_buffer_bindings[target] = buffer;
}

void PS4_SYSV_ABI glBindBufferBase(u32 target, u32 index, u32 buffer) {
    if (GlContext* gl = RequireContext()) {
        BindIndexedBuffer(*gl, target, index, buffer, 0, 0);
    }
}

void PS4_SYSV_ABI glBindBufferRange(u32 target, u32 index, u32 buffer, s64 offset, s64 size) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (buffer != 0 && (size <= 0 || offset < 0 || (target == 0x8A11 && offset % 4 != 0) ||
                        (target == 0x8C8E && (offset % 4 != 0 || size % 4 != 0)))) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    BindIndexedBuffer(*gl, target, index, buffer, offset, size);
}

void PS4_SYSV_ABI glGetIntegeri_v(u32 target, u32 index, s32* data) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const bool uniform = target >= 0x8A28 && target <= 0x8A2A;
    const bool feedback = target == 0x8C8F || target == 0x8C84 || target == 0x8C85;
    if (!uniform && !feedback) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    const GlIndexedBinding* binding = nullptr;
    if (uniform && index < MAX_UNIFORM_BUFFER_BINDINGS) {
        binding = &gl->uniform_buffer_bindings[index];
    } else if (feedback && index < MAX_TRANSFORM_FEEDBACK_ATTRIBS) {
        binding = &gl->transform_feedback_bindings[index];
    }
    if (binding == nullptr) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    switch (target) {
    case 0x8A28:
    case 0x8C8F:
        *data = static_cast<s32>(binding->buffer);
        break;
    case 0x8A29:
    case 0x8C84:
        *data = static_cast<s32>(binding->offset);
        break;
    default:
        *data = static_cast<s32>(binding->size);
        break;
    }
}

void PS4_SYSV_ABI glGetInteger64i_v(u32 target, u32 index, s64* data) {
    s32 value = 0;
    glGetIntegeri_v(target, index, &value);
    *data = value;
}

void PS4_SYSV_ABI glBeginQuery(u32 target, u32 id) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsQueryTarget(target)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (id == 0 || gl->active_queries.contains(target)) {
        SetGlError(*gl, id == 0 ? GL_INVALID_OPERATION : GL_INVALID_OPERATION);
        return;
    }
    GlQuery& query = gl->queries.Bind(id);
    if (query.target != 0 && query.target != target) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    query.target = target;
    query.active = true;
    gl->active_queries[target] = id;
}

void PS4_SYSV_ABI glEndQuery(u32 target) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsQueryTarget(target)) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    const auto it = gl->active_queries.find(target);
    if (it == gl->active_queries.end()) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    if (GlQuery* query = gl->queries.Find(it->second)) {
        query->active = false;
    }
    gl->active_queries.erase(it);
}

void PS4_SYSV_ABI glGetQueryiv(u32 target, u32 pname, s32* params) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (!IsQueryTarget(target) && target != 0x8E28) { // GL_TIMESTAMP_EXT
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (pname == 0x8865) { // GL_CURRENT_QUERY
        const auto it = gl->active_queries.find(target);
        *params = it != gl->active_queries.end() ? static_cast<s32>(it->second) : 0;
    } else if (pname == 0x8864) { // GL_QUERY_COUNTER_BITS_EXT
        // TODO: The counter size depends on the GPU timestamp support.
        *params = 0;
    } else {
        SetGlError(*gl, GL_INVALID_ENUM);
    }
}

// Reads a query object. Without a rendering backend every query completes immediately with a
// result of zero.
static void GetQueryObject(u32 id, u32 pname, s64& value) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const GlQuery* query = gl->queries.Find(id);
    if (query == nullptr || query->active) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    if (pname == 0x8867) { // GL_QUERY_RESULT_AVAILABLE
        value = 1;
    } else if (pname == 0x8866) { // GL_QUERY_RESULT
        value = 0;
    } else {
        SetGlError(*gl, GL_INVALID_ENUM);
    }
}

void PS4_SYSV_ABI glGetQueryObjectuiv(u32 id, u32 pname, u32* params) {
    s64 value = 0;
    GetQueryObject(id, pname, value);
    *params = static_cast<u32>(value);
}

void PS4_SYSV_ABI glGenQueriesEXT(s32 n, u32* ids) {
    glGenQueries(n, ids);
}

void PS4_SYSV_ABI glDeleteQueriesEXT(s32 n, const u32* ids) {
    glDeleteQueries(n, ids);
}

u8 PS4_SYSV_ABI glIsQueryEXT(u32 id) {
    return glIsQuery(id);
}

void PS4_SYSV_ABI glBeginQueryEXT(u32 target, u32 id) {
    glBeginQuery(target, id);
}

void PS4_SYSV_ABI glEndQueryEXT(u32 target) {
    glEndQuery(target);
}

void PS4_SYSV_ABI glQueryCounterEXT(u32 id, u32 target) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != 0x8E28) { // GL_TIMESTAMP_EXT
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (id == 0) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    gl->queries.Bind(id).target = target;
}

void PS4_SYSV_ABI glGetQueryObjectivEXT(u32 id, u32 pname, s32* params) {
    s64 value = 0;
    GetQueryObject(id, pname, value);
    *params = static_cast<s32>(value);
}

void PS4_SYSV_ABI glGetQueryObjectuivEXT(u32 id, u32 pname, u32* params) {
    glGetQueryObjectuiv(id, pname, params);
}

void PS4_SYSV_ABI glGetQueryObjecti64vEXT(u32 id, u32 pname, s64* params) {
    s64 value = 0;
    GetQueryObject(id, pname, value);
    *params = value;
}

void PS4_SYSV_ABI glGetQueryObjectui64vEXT(u32 id, u32 pname, u64* params) {
    s64 value = 0;
    GetQueryObject(id, pname, value);
    *params = static_cast<u64>(value);
}

void PS4_SYSV_ABI glBindTransformFeedback(u32 target, u32 id) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (target != 0x8E22) { // GL_TRANSFORM_FEEDBACK
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    const GlTransformFeedback& current = CurrentTransformFeedback(*gl);
    if (current.active && !current.paused) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    if (id != 0) {
        gl->transform_feedbacks.Bind(id);
    }
    gl->transform_feedback = id;
}

void PS4_SYSV_ABI glBeginTransformFeedback(u32 primitive_mode) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (primitive_mode != 0 && primitive_mode != 1 && primitive_mode != 4) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    GlTransformFeedback& feedback = CurrentTransformFeedback(*gl);
    if (feedback.active || gl->current_program == 0) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    feedback.active = true;
    feedback.paused = false;
}

void PS4_SYSV_ABI glEndTransformFeedback() {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    GlTransformFeedback& feedback = CurrentTransformFeedback(*gl);
    if (!feedback.active) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    feedback.active = false;
    feedback.paused = false;
}

void PS4_SYSV_ABI glPauseTransformFeedback() {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    GlTransformFeedback& feedback = CurrentTransformFeedback(*gl);
    if (!feedback.active || feedback.paused) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    feedback.paused = true;
}

void PS4_SYSV_ABI glResumeTransformFeedback() {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    GlTransformFeedback& feedback = CurrentTransformFeedback(*gl);
    if (!feedback.active || !feedback.paused) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    feedback.paused = false;
}

void PS4_SYSV_ABI glTransformFeedbackVaryings(u32 program, s32 count, const char* const* varyings,
                                              u32 buffer_mode) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    GlProgram* prog = FindProgram(*gl, program);
    if (prog == nullptr) {
        return;
    }
    if (buffer_mode != 0x8C8C && buffer_mode != 0x8C8D) { // INTERLEAVED, SEPARATE
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    if (count < 0 ||
        (buffer_mode == 0x8C8D && count > static_cast<s32>(MAX_TRANSFORM_FEEDBACK_ATTRIBS))) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    prog->transform_feedback_varyings.assign(varyings, varyings + count);
    prog->transform_feedback_buffer_mode = buffer_mode;
}

void PS4_SYSV_ABI glGetTransformFeedbackVarying(u32 program, u32 index, s32 buf_size, s32* length,
                                                s32* size, u32* type, char* name) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const GlProgram* prog = FindProgram(*gl, program);
    if (prog == nullptr) {
        return;
    }
    if (buf_size < 0 || index >= prog->transform_feedback_varyings.size()) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    // TODO: The size and type need program reflection.
    CopyString(prog->transform_feedback_varyings[index], buf_size, length, name);
    if (size != nullptr) {
        *size = 1;
    }
    if (type != nullptr) {
        *type = 0x1406; // GL_FLOAT
    }
}

void PS4_SYSV_ABI glBindSampler(u32 unit, u32 sampler) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    if (unit >= MAX_TEXTURE_UNITS) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    if (sampler != 0) {
        gl->samplers.Bind(sampler);
    }
}

// Applies a sampler parameter after checking the sampler object.
static void SetSamplerParam(u32 sampler, u32 pname, double value) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    GlSampler* object = gl->samplers.FindGenerated(sampler);
    if (object == nullptr) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    if (!IsSamplerParam(pname) || pname == 0x84FE) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    SetTextureParam(*gl, *object, pname, value);
}

void PS4_SYSV_ABI glSamplerParameteri(u32 sampler, u32 pname, s32 param) {
    SetSamplerParam(sampler, pname, param);
}

void PS4_SYSV_ABI glSamplerParameteriv(u32 sampler, u32 pname, const s32* param) {
    SetSamplerParam(sampler, pname, param[0]);
}

void PS4_SYSV_ABI glSamplerParameterf(u32 sampler, u32 pname, float param) {
    SetSamplerParam(sampler, pname, param);
}

void PS4_SYSV_ABI glSamplerParameterfv(u32 sampler, u32 pname, const float* param) {
    SetSamplerParam(sampler, pname, param[0]);
}

void PS4_SYSV_ABI glGetSamplerParameteriv(u32 sampler, u32 pname, s32* params) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const GlSampler* object = gl->samplers.FindGenerated(sampler);
    if (object == nullptr) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    if (!IsSamplerParam(pname) || pname == 0x84FE) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    *params = ToInteger(GetTextureParam(*object, pname));
}

void PS4_SYSV_ABI glGetSamplerParameterfv(u32 sampler, u32 pname, float* params) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    const GlSampler* object = gl->samplers.FindGenerated(sampler);
    if (object == nullptr) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        return;
    }
    if (!IsSamplerParam(pname) || pname == 0x84FE) {
        SetGlError(*gl, GL_INVALID_ENUM);
        return;
    }
    *params = static_cast<float>(GetTextureParam(*object, pname));
}

// Debug markers do not change any state.
void PS4_SYSV_ABI glInsertEventMarkerEXT(s32 length, const char* marker) {}

void PS4_SYSV_ABI glPushGroupMarkerEXT(s32 length, const char* marker) {}

void PS4_SYSV_ABI glPopGroupMarkerEXT() {}

void PS4_SYSV_ABI glInsertEventMarkerColorSCE(s32 length, const char* marker, u32 color) {}

void PS4_SYSV_ABI glPushGroupMarkerColorSCE(s32 length, const char* marker, u32 color) {}

u32 PS4_SYSV_ABI glPigletGetShaderBinarySCE(u32 shader, s32 buf_size, s32* length,
                                            u32* binary_format, void* binary) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return 0;
    }
    if (binary == nullptr || binary_format == nullptr) {
        SetGlError(*gl, GL_INVALID_VALUE);
        LOG_ERROR(Lib_Piglet, "Invalid parameters!");
        return 0;
    }
    const auto it = gl->shaders.find(shader);
    if (it == gl->shaders.end()) {
        SetGlError(*gl, GL_INVALID_VALUE);
        LOG_ERROR(Lib_Piglet, "Invalid shader!");
        return 0;
    }
    const GlShader& object = it->second;
    if (!object.compiled || buf_size < 0 || static_cast<size_t>(buf_size) < object.binary.size()) {
        SetGlError(*gl, GL_INVALID_OPERATION);
        LOG_ERROR(Lib_Piglet, "Failed to get shader binary!");
        return 0;
    }
    std::memcpy(binary, object.binary.data(), object.binary.size());
    if (length != nullptr) {
        *length = static_cast<s32>(object.binary.size());
    }
    // GL_SCE_piglet_shader_binary
    *binary_format = 0x9270;
    return 1;
}

void PS4_SYSV_ABI _OrbisTextureImage2DCanvas(u32 target, u32 canvas) {
    GlContext* gl = RequireContext();
    if (gl == nullptr) {
        return;
    }
    // Targets Piglet accepts: 2D, 3D, 2D array, cube map (and array) and 2D multisample.
    const bool valid = target == GL_TEXTURE_2D || target == 0x806F || target == 0x8C1A ||
                       target == 0x8513 || target == 0x9009 || target == 0x9100;
    if (!valid) {
        SetGlError(*gl, GL_INVALID_VALUE);
        return;
    }
    // TODO: The texture is meant to display a compositor canvas. There is no compositor, so only
    // the association is stored.
    GlTexture& texture = *BoundTexture(*gl, target);
    texture.is_canvas = true;
    texture.canvas_handle = canvas;
}

void RegisterGles(Core::Loader::SymbolsResolver* sym) {
    PIGLET_EXPORT("y20mWUHGzj8", glActiveTexture);
    PIGLET_EXPORT("uVdONJ4yNrc", glAttachShader);
    PIGLET_EXPORT("lY7qdFGr0go", glBindAttribLocation);
    PIGLET_EXPORT("nl8lQxwAceM", glBindBuffer);
    PIGLET_EXPORT("vWkaB9NZpGA", glBindFramebuffer);
    PIGLET_EXPORT("lGJTnZqchvA", glBindRenderbuffer);
    PIGLET_EXPORT("I9IXZ61WkD0", glBindTexture);
    PIGLET_EXPORT("ey9N5U-C0hw", glBlendColor);
    PIGLET_EXPORT("HFIXBmlQmXI", glBlendEquation);
    PIGLET_EXPORT("3yQipX6ypKM", glBlendEquationSeparate);
    PIGLET_EXPORT("39BK09X7zLY", glBlendFunc);
    PIGLET_EXPORT("En8ghzCM0HM", glBlendFuncSeparate);
    PIGLET_EXPORT("hKN5OJIT5ko", glBufferData);
    PIGLET_EXPORT("9rH9n8530IU", glBufferSubData);
    PIGLET_EXPORT("-HAzeV7vL-Q", glCheckFramebufferStatus);
    PIGLET_EXPORT("KOJ4+zzhpAg", glClear);
    PIGLET_EXPORT("Pkhe5Qcq++0", glClearColor);
    PIGLET_EXPORT("s5sv2KhvlxE", glClearDepthf);
    PIGLET_EXPORT("QBcAfwrr094", glClearStencil);
    PIGLET_EXPORT("XQZkyJMxI18", glColorMask);
    PIGLET_EXPORT("zyebgmQUnWY", glCompileShader);
    PIGLET_EXPORT("KZ0xIC-Ev2c", glCompressedTexImage2D);
    PIGLET_EXPORT("XZB2-9dSghM", glCompressedTexSubImage2D);
    PIGLET_EXPORT("yyJshE4eV3U", glCopyTexImage2D);
    PIGLET_EXPORT("MnlqH3VAJxU", glCopyTexSubImage2D);
    PIGLET_EXPORT("q0iQ6QttHjQ", glCreateProgram);
    PIGLET_EXPORT("0TyJ6AxRs5w", glCreateShader);
    PIGLET_EXPORT("bUxXBzkgul0", glCullFace);
    PIGLET_EXPORT("mPRFA5CE48g", glDeleteBuffers);
    PIGLET_EXPORT("3xsE3RDNnQw", glDeleteFramebuffers);
    PIGLET_EXPORT("oOigDJBsN2c", glDeleteProgram);
    PIGLET_EXPORT("CWnGG3DOIt8", glDeleteRenderbuffers);
    PIGLET_EXPORT("r5IsX1dPwUk", glDeleteShader);
    PIGLET_EXPORT("tZ1lckB5o2A", glDeleteTextures);
    PIGLET_EXPORT("FyhlwO6-+sM", glDepthFunc);
    PIGLET_EXPORT("aN1B2MP3kLU", glDepthMask);
    PIGLET_EXPORT("B-mNueVFgAw", glDepthRangef);
    PIGLET_EXPORT("RJtXkpCVfmA", glDetachShader);
    PIGLET_EXPORT("pwl29nuNnqI", glDisable);
    PIGLET_EXPORT("BpP68iXiK5I", glDisableVertexAttribArray);
    PIGLET_EXPORT("19N0joauSsE", glDrawArrays);
    PIGLET_EXPORT("Xe-vzCJeV3Y", glDrawElements);
    PIGLET_EXPORT("TIdn+yGbJlo", glEnable);
    PIGLET_EXPORT("sA3IshgN8u8", glEnableVertexAttribArray);
    PIGLET_EXPORT("APjMbicny1Q", glFinish);
    PIGLET_EXPORT("VPgPm-9j054", glFlush);
    PIGLET_EXPORT("YjT8GrH5bsM", glFramebufferRenderbuffer);
    PIGLET_EXPORT("326-26W1Ht4", glFramebufferTexture2D);
    PIGLET_EXPORT("4Pd54jvwVhY", glFrontFace);
    PIGLET_EXPORT("4p8IUn5CM0I", glGenBuffers);
    PIGLET_EXPORT("nmUsbuL3Rc0", glGenerateMipmap);
    PIGLET_EXPORT("6OfeCf-u4bg", glGenFramebuffers);
    PIGLET_EXPORT("ZggLwqNsGIk", glGenRenderbuffers);
    PIGLET_EXPORT("OR6-CZL+oOw", glGenTextures);
    PIGLET_EXPORT("kF-yS5OChO8", glGetActiveAttrib);
    PIGLET_EXPORT("tY7yQOR9-8s", glGetActiveUniform);
    PIGLET_EXPORT("Bc3rL5ELD-A", glGetAttachedShaders);
    PIGLET_EXPORT("beY9ZJv5-dU", glGetAttribLocation);
    PIGLET_EXPORT("tm+Ktm7aelA", glGetBooleanv);
    PIGLET_EXPORT("TgsW7eXEdpA", glGetBufferParameteriv);
    PIGLET_EXPORT("uyjjXGqy-Ek", glGetError);
    PIGLET_EXPORT("Gfzd1R86ep8", glGetFloatv);
    PIGLET_EXPORT("sw2+TzZR71g", glGetFramebufferAttachmentParameteriv);
    PIGLET_EXPORT("-CyF1XyqmaQ", glGetIntegerv);
    PIGLET_EXPORT("m-7cQfah6KY", glGetProgramiv);
    PIGLET_EXPORT("uVoPGNUyeZU", glGetProgramInfoLog);
    PIGLET_EXPORT("jzjrS8gGNdY", glGetRenderbufferParameteriv);
    PIGLET_EXPORT("sfRsq9Yq2Y8", glGetShaderiv);
    PIGLET_EXPORT("Yo4iw4JYCTI", glGetShaderInfoLog);
    PIGLET_EXPORT("DRwo4pgCHKk", glGetShaderPrecisionFormat);
    PIGLET_EXPORT("2-+fJU7sdx8", glGetShaderSource);
    PIGLET_EXPORT("xqbiS7PMrSE", glGetString);
    PIGLET_EXPORT("mPN01lKHg0g", glGetTexParameterfv);
    PIGLET_EXPORT("Vov9fE6FD1c", glGetTexParameteriv);
    PIGLET_EXPORT("q3nWOTJHIM4", glGetUniformfv);
    PIGLET_EXPORT("ArYdZGEUW8w", glGetUniformiv);
    PIGLET_EXPORT("UMeDmpCpF40", glGetUniformLocation);
    PIGLET_EXPORT("yyAMU7O65qY", glGetVertexAttribfv);
    PIGLET_EXPORT("gaFx7Axnsuo", glGetVertexAttribiv);
    PIGLET_EXPORT("ffv6I7TFVP4", glGetVertexAttribPointerv);
    PIGLET_EXPORT("SCdKl+0EnuQ", glHint);
    PIGLET_EXPORT("nd-xdRd4+BU", glIsBuffer);
    PIGLET_EXPORT("Cu07x4fIYO0", glIsEnabled);
    PIGLET_EXPORT("S7VZWmI1u04", glIsFramebuffer);
    PIGLET_EXPORT("1hulS01us-4", glIsProgram);
    PIGLET_EXPORT("fVpuh+UJ4Qo", glIsRenderbuffer);
    PIGLET_EXPORT("g-XVEn-57y8", glIsShader);
    PIGLET_EXPORT("p8hjICuQP74", glIsTexture);
    PIGLET_EXPORT("nMoFUeBjzrU", glLineWidth);
    PIGLET_EXPORT("ON26vhraMMU", glLinkProgram);
    PIGLET_EXPORT("GNZpcrnC7uE", glPixelStorei);
    PIGLET_EXPORT("LJ9eQ5Z7Wmo", glPolygonOffset);
    PIGLET_EXPORT("tPHxFS+55pY", glReadPixels);
    PIGLET_EXPORT("9TZg6EG5ncM", glReleaseShaderCompiler);
    PIGLET_EXPORT("2jKVW2uBSw8", glRenderbufferStorage);
    PIGLET_EXPORT("LFLRtqqi4AQ", glSampleCoverage);
    PIGLET_EXPORT("GmP2ZgUoTPo", glScissor);
    PIGLET_EXPORT("yFJY7Te6ZRQ", glShaderBinary);
    PIGLET_EXPORT("2PuCkkEw9eE", glShaderSource);
    PIGLET_EXPORT("hZarPUiZevM", glStencilFunc);
    PIGLET_EXPORT("6z5ySLhOFQE", glStencilFuncSeparate);
    PIGLET_EXPORT("jGo2URP56tE", glStencilMask);
    PIGLET_EXPORT("1Tw-GwQJvug", glStencilMaskSeparate);
    PIGLET_EXPORT("t8HI9sZ4TSM", glStencilOp);
    PIGLET_EXPORT("mTcFOuWRrhw", glStencilOpSeparate);
    PIGLET_EXPORT("52+a3D15aw4", glTexImage2D);
    PIGLET_EXPORT("1dtq0xFWSNU", glTexParameterf);
    PIGLET_EXPORT("ehXUKUtch8M", glTexParameterfv);
    PIGLET_EXPORT("Kzbvd9G7ieE", glTexParameteri);
    PIGLET_EXPORT("6e64qI81ak8", glTexParameteriv);
    PIGLET_EXPORT("3DK2dxDedi0", glTexSubImage2D);
    PIGLET_EXPORT("rTb6vWn9eL0", glTextureStorage2DEXT);
    PIGLET_EXPORT("xiH4YdjKEYc", glTexStorage2DEXT);
    PIGLET_EXPORT("POq8+5-inHY", glUniform1f);
    PIGLET_EXPORT("Dn1NY1xs7Zw", glUniform1fv);
    PIGLET_EXPORT("hR59aDWzHPw", glUniform1i);
    PIGLET_EXPORT("DqJyaSqqTng", glUniform1iv);
    PIGLET_EXPORT("e5HOpV5feq8", glUniform2f);
    PIGLET_EXPORT("U6zCblLII4w", glUniform2fv);
    PIGLET_EXPORT("qC8BIIGtfNk", glUniform2i);
    PIGLET_EXPORT("416vjcKYsfo", glUniform2iv);
    PIGLET_EXPORT("I82nyOgGKjY", glUniform3f);
    PIGLET_EXPORT("4hwL0vHb1Jw", glUniform3fv);
    PIGLET_EXPORT("zmymoDNsdKM", glUniform3i);
    PIGLET_EXPORT("vdV5zDWJ9Q8", glUniform3iv);
    PIGLET_EXPORT("H8Dp8lwh8QQ", glUniform4f);
    PIGLET_EXPORT("tZSQ+VFbsxE", glUniform4fv);
    PIGLET_EXPORT("YWItHEulIAI", glUniform4i);
    PIGLET_EXPORT("WS+7Pxb5gNg", glUniform4iv);
    PIGLET_EXPORT("oaqISo2f6rk", glUniformMatrix2fv);
    PIGLET_EXPORT("7w09iXWXQZc", glUniformMatrix3fv);
    PIGLET_EXPORT("v8-KyLvjWKw", glUniformMatrix4fv);
    PIGLET_EXPORT("ADsMyXKjHVo", glUseProgram);
    PIGLET_EXPORT("Yymje0BT12Y", glValidateProgram);
    PIGLET_EXPORT("vSCc5IpzQ-w", glVertexAttrib1f);
    PIGLET_EXPORT("syWd-JCitpo", glVertexAttrib1fv);
    PIGLET_EXPORT("JEt20zWYCIE", glVertexAttrib2f);
    PIGLET_EXPORT("tqnmnBCbNZI", glVertexAttrib2fv);
    PIGLET_EXPORT("SOZzgxqVHIY", glVertexAttrib3f);
    PIGLET_EXPORT("qU9JeLiphww", glVertexAttrib3fv);
    PIGLET_EXPORT("ub4yceOLlbQ", glVertexAttrib4f);
    PIGLET_EXPORT("Ig0Ts6ju8rQ", glVertexAttrib4fv);
    PIGLET_EXPORT("Yk0ofesYP1U", glVertexAttribPointer);
    PIGLET_EXPORT("idWUOMHlXf0", glViewport);
    PIGLET_EXPORT("68Xne9xAk7s", glReadBuffer);
    PIGLET_EXPORT("XS1XH14cemc", glDrawArraysInstanced);
    PIGLET_EXPORT("5I0ZSvxhRmc", glDrawElementsInstanced);
    PIGLET_EXPORT("camy5cvmZII", glDrawRangeElements);
    PIGLET_EXPORT("zJ8P2q3ZflM", glTexImage3D);
    PIGLET_EXPORT("jCksK8VIzyE", glTexSubImage3D);
    PIGLET_EXPORT("piWdZNlOTLo", glCopyTexSubImage3D);
    PIGLET_EXPORT("AoKQpye80kU", glCompressedTexImage3D);
    PIGLET_EXPORT("igpSrATXmAY", glCompressedTexSubImage3D);
    PIGLET_EXPORT("h0slGrL6pBs", glGenQueries);
    PIGLET_EXPORT("OHKoGVZ7xKA", glDeleteQueries);
    PIGLET_EXPORT("Ads7t69vHBk", glIsQuery);
    PIGLET_EXPORT("CNx0ylkWoCg", glBeginQuery);
    PIGLET_EXPORT("wA0LYGRV2KQ", glEndQuery);
    PIGLET_EXPORT("yiOOLA2vkLA", glGetQueryiv);
    PIGLET_EXPORT("rJWnDO35mR8", glGetQueryObjectuiv);
    PIGLET_EXPORT("1hAznJNzbI8", glUnmapBuffer);
    PIGLET_EXPORT("D0n924iO0Yw", glGetBufferPointerv);
    PIGLET_EXPORT("GrLInwUuV2k", glDrawBuffers);
    PIGLET_EXPORT("3DtWT550eGM", glUniformMatrix2x3fv);
    PIGLET_EXPORT("4MR1gw+ffTs", glUniformMatrix3x2fv);
    PIGLET_EXPORT("kOZTCBZCk4g", glUniformMatrix2x4fv);
    PIGLET_EXPORT("6II7YIgxbSU", glUniformMatrix4x2fv);
    PIGLET_EXPORT("b-YdE60Z8tw", glUniformMatrix3x4fv);
    PIGLET_EXPORT("rD1uDD2rEso", glUniformMatrix4x3fv);
    PIGLET_EXPORT("M27ezF1iANs", glBlitFramebuffer);
    PIGLET_EXPORT("4M5OQvD2uE8", glRenderbufferStorageMultisample);
    PIGLET_EXPORT("NHxpE45ygfQ", glFramebufferTextureLayer);
    PIGLET_EXPORT("V6fVkEahkLM", glMapBufferRange);
    PIGLET_EXPORT("wGn8sX26Fds", glFlushMappedBufferRange);
    PIGLET_EXPORT("f8f0+hYyOb0", glBindVertexArray);
    PIGLET_EXPORT("En+dYUM8u7U", glDeleteVertexArrays);
    PIGLET_EXPORT("Ai5yrljUSjI", glGenVertexArrays);
    PIGLET_EXPORT("G2U3S-VCv-c", glIsVertexArray);
    PIGLET_EXPORT("gTD5beqJ81U", glGetIntegeri_v);
    PIGLET_EXPORT("2w3bo8DF4vM", glBeginTransformFeedback);
    PIGLET_EXPORT("1ELoOr5IBlM", glEndTransformFeedback);
    PIGLET_EXPORT("v3Qv1I3P7x4", glBindBufferRange);
    PIGLET_EXPORT("UFb4eVjw-Sw", glBindBufferBase);
    PIGLET_EXPORT("jQlQ4IR95YU", glTransformFeedbackVaryings);
    PIGLET_EXPORT("5dfVNhCZFxE", glGetTransformFeedbackVarying);
    PIGLET_EXPORT("Cyw+pgg2YcI", glVertexAttribIPointer);
    PIGLET_EXPORT("x8npgO9yaNE", glGetVertexAttribIiv);
    PIGLET_EXPORT("LVLwSxJwwnw", glGetVertexAttribIuiv);
    PIGLET_EXPORT("xMlxiPxwrAw", glVertexAttribI4i);
    PIGLET_EXPORT("n79wW64CUh8", glVertexAttribI4ui);
    PIGLET_EXPORT("VWq4YfFcCz0", glVertexAttribI4iv);
    PIGLET_EXPORT("rhmQ142bvvU", glVertexAttribI4uiv);
    PIGLET_EXPORT("C3vniWm8dNw", glGetUniformuiv);
    PIGLET_EXPORT("wGe2stW-CTw", glGetFragDataLocation);
    PIGLET_EXPORT("XkZ7aJ4mICw", glUniform1ui);
    PIGLET_EXPORT("-4nCkJB-Wpk", glUniform2ui);
    PIGLET_EXPORT("rP1Z5VoDsWw", glUniform3ui);
    PIGLET_EXPORT("aOKKRhOYzyU", glUniform4ui);
    PIGLET_EXPORT("4vHxgXZ9HV8", glUniform1uiv);
    PIGLET_EXPORT("GE+2k2MFilI", glUniform2uiv);
    PIGLET_EXPORT("RUbJ1HDWEHs", glUniform3uiv);
    PIGLET_EXPORT("Dm9kHKVv-GI", glUniform4uiv);
    PIGLET_EXPORT("Untg89NcROM", glClearBufferiv);
    PIGLET_EXPORT("5CL3pzRNzJ4", glClearBufferuiv);
    PIGLET_EXPORT("g-C5shOvH1Y", glClearBufferfv);
    PIGLET_EXPORT("J06J7hH+9bs", glClearBufferfi);
    PIGLET_EXPORT("OOI5nqLihBw", glGetStringi);
    PIGLET_EXPORT("l1PatT2jaok", glCopyBufferSubData);
    PIGLET_EXPORT("Ok3Hu4DWmpU", glGetUniformIndices);
    PIGLET_EXPORT("Os1rxlZYt-Y", glGetActiveUniformsiv);
    PIGLET_EXPORT("MyyhVXryovc", glGetUniformBlockIndex);
    PIGLET_EXPORT("-+8sSDc61uU", glGetActiveUniformBlockiv);
    PIGLET_EXPORT("KDPOldWSEtc", glGetActiveUniformBlockName);
    PIGLET_EXPORT("qSbpBon7f2s", glUniformBlockBinding);
    PIGLET_EXPORT("PdWicphhWgg", glFenceSync);
    PIGLET_EXPORT("2m41tMRyPpc", glIsSync);
    PIGLET_EXPORT("eelf17HNGa4", glDeleteSync);
    PIGLET_EXPORT("UKSkPc4YLt8", glClientWaitSync);
    PIGLET_EXPORT("Ab0R4oOE-DY", glWaitSync);
    PIGLET_EXPORT("j1qEC6CZgpQ", glGetSynciv);
    PIGLET_EXPORT("4e-JjxDUlb8", glGetInteger64v);
    PIGLET_EXPORT("R3grMrGyJho", glGetInteger64i_v);
    PIGLET_EXPORT("5T6TphmcgVQ", glGetBufferParameteri64v);
    PIGLET_EXPORT("B3wg5t9bCdk", glGenSamplers);
    PIGLET_EXPORT("dI1Y-XIAEqU", glDeleteSamplers);
    PIGLET_EXPORT("SOMZ080N0wA", glIsSampler);
    PIGLET_EXPORT("9G2LYp3FlE4", glBindSampler);
    PIGLET_EXPORT("AsueLb+3C4k", glSamplerParameteri);
    PIGLET_EXPORT("9h3QG13-rQQ", glSamplerParameteriv);
    PIGLET_EXPORT("fbiTO9gjGSE", glSamplerParameterf);
    PIGLET_EXPORT("gAT2BtxphDM", glSamplerParameterfv);
    PIGLET_EXPORT("r-OiMbqJuEQ", glGetSamplerParameteriv);
    PIGLET_EXPORT("CWOI5J2vrQo", glGetSamplerParameterfv);
    PIGLET_EXPORT("5a4nrywbfZc", glBindTransformFeedback);
    PIGLET_EXPORT("uUjwGu7aZuU", glDeleteTransformFeedbacks);
    PIGLET_EXPORT("+mwnHP7Lou8", glGenTransformFeedbacks);
    PIGLET_EXPORT("VBNYE1MPf4I", glIsTransformFeedback);
    PIGLET_EXPORT("VXxylRE34KU", glPauseTransformFeedback);
    PIGLET_EXPORT("CAypDWjJScU", glResumeTransformFeedback);
    PIGLET_EXPORT("1p5uDHGLvd4", glGetProgramBinary);
    PIGLET_EXPORT("mr4YjdpfVFg", glProgramBinary);
    PIGLET_EXPORT("0kYe0bK58zk", glProgramParameteri);
    PIGLET_EXPORT("I+2QnmhN3xc", glInvalidateFramebuffer);
    PIGLET_EXPORT("+caGEFeVZd8", glInvalidateSubFramebuffer);
    PIGLET_EXPORT("LGiUQAdAxTQ", glTexStorage2D);
    PIGLET_EXPORT("-y-tRnUpuVE", glTexStorage3D);
    PIGLET_EXPORT("56f34U0xe8c", glGetInternalformativ);
    PIGLET_EXPORT("rSW2hZt1Oo4", glVertexAttribDivisor);
    PIGLET_EXPORT("PWEUl-4tXqg", glPigletGetShaderBinarySCE);
    PIGLET_EXPORT("F35Uj-GMte0", glDrawArraysInstancedEXT);
    PIGLET_EXPORT("3Ns42AD8UGI", glDrawElementsInstancedEXT);
    PIGLET_EXPORT("KGXoebAph80", glVertexAttribDivisorEXT);
    PIGLET_EXPORT("R+lZMziFVTQ", glGenQueriesEXT);
    PIGLET_EXPORT("d38bTmxILUQ", glDeleteQueriesEXT);
    PIGLET_EXPORT("5HIn4jxKwu8", glIsQueryEXT);
    PIGLET_EXPORT("UkvXd2jt++I", glBeginQueryEXT);
    PIGLET_EXPORT("U6+U2AYUe7Y", glEndQueryEXT);
    PIGLET_EXPORT("RWc9kPMyEYU", glQueryCounterEXT);
    PIGLET_EXPORT("CqW1XfJDqNs", glGetQueryObjectivEXT);
    PIGLET_EXPORT("go0QBZdmPKk", glGetQueryObjectuivEXT);
    PIGLET_EXPORT("WIGwUmVdp+Y", glGetQueryObjectui64vEXT);
    PIGLET_EXPORT("zyTMmUiR2tQ", glGetQueryObjecti64vEXT);
    PIGLET_EXPORT("w2aPcrilqmg", glInsertEventMarkerEXT);
    PIGLET_EXPORT("fo7GYqkkHBA", glPushGroupMarkerEXT);
    PIGLET_EXPORT("3QC1YQvFxEw", glPopGroupMarkerEXT);
    PIGLET_EXPORT("YdGQOl6bp50", glInsertEventMarkerColorSCE);
    PIGLET_EXPORT("xaQHq4LK5zQ", glPushGroupMarkerColorSCE);
    PIGLET_EXPORT("B62IAjtQimk", _OrbisTextureImage2DCanvas);
}

} // namespace Libraries::Piglet
