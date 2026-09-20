// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

class FileIncluder final : public glslang::TShader::Includer {
public:
    IncludeResult* includeLocal(const char* header_name, const char* includer_name,
                                std::size_t inclusion_depth) override {
        if (inclusion_depth > 64) {
            return nullptr;
        }

        const auto path =
            (std::filesystem::path{includer_name}.parent_path() / header_name).lexically_normal();
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            return nullptr;
        }

        auto* contents = new std::string{std::istreambuf_iterator<char>{input}, {}};
        if (input.bad()) {
            throw std::runtime_error("Cannot read " + path.string());
        }

        dependencies.insert(path);
        return new IncludeResult{path.generic_string(), contents->data(), contents->size(),
                                 contents};
    }

    void releaseInclude(IncludeResult* result) override {
        if (result) {
            delete static_cast<std::string*>(result->userData);
            delete result;
        }
    }

    std::set<std::filesystem::path> dependencies;
};

std::string EscapeDepfilePath(const std::filesystem::path& path) {
    std::string escaped;
    for (char c : path.generic_string()) {
        if (c == '$') {
            escaped += "$$";
        } else {
            if (c == ' ' || c == '\t' || c == '#' || c == '\\' || c == ':') {
                escaped += '\\';
            }
            escaped += c;
        }
    }
    return escaped;
}

void WriteDepfile(const std::filesystem::path& depfile, const std::filesystem::path& header,
                  const std::set<std::filesystem::path>& dependencies) {
    std::ofstream output;
    output.exceptions(std::ios::failbit | std::ios::badbit);
    output.open(depfile, std::ios::binary);
    output << EscapeDepfilePath(header) << ':';
    for (const auto& dependency : dependencies) {
        output << ' ' << EscapeDepfilePath(dependency);
    }
    output << '\n';
    output.close();
}

EShLanguage ShaderStage(const std::filesystem::path& path) {
    const auto extension = path.extension();
    if (extension == ".vert") {
        return EShLangVertex;
    }
    if (extension == ".frag") {
        return EShLangFragment;
    }
    if (extension == ".comp") {
        return EShLangCompute;
    }
    throw std::runtime_error("Unsupported shader stage: " + path.string());
}

void Compile(int argc, char** argv) {
    const auto source_path = std::filesystem::absolute(argv[1]).lexically_normal();
    std::ifstream input(source_path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Cannot read " + source_path.string());
    }

    const std::string source{std::istreambuf_iterator<char>{input}, {}};
    const char* source_data = source.data();
    const int source_size = static_cast<int>(source.size());
    const auto source_name = source_path.generic_string();
    const char* source_name_data = source_name.c_str();
    const auto stage = ShaderStage(source_path);

    FileIncluder includer;
    includer.dependencies.insert(source_path);
    glslang::TShader shader{stage};
    shader.setStringsWithLengthsAndNames(&source_data, &source_size, &source_name_data, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

    std::string preamble;
    for (int i = 5; i < argc; ++i) {
        std::string define{argv[i]};
        if (const auto equal = define.find('='); equal != std::string::npos) {
            define[equal] = ' ';
        }
        preamble += "#define " + define + "\n";
    }
    shader.setPreamble(preamble.c_str());

    const auto messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
    if (!shader.parse(GetDefaultResources(), 450, false, messages, includer)) {
        throw std::runtime_error(std::string{shader.getInfoLog()} + shader.getInfoDebugLog());
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages)) {
        throw std::runtime_error(std::string{program.getInfoLog()} + program.getInfoDebugLog());
    }

    std::vector<std::uint32_t> spirv;
    spv::SpvBuildLogger logger;
    glslang::SpvOptions options;
    options.disableOptimizer = false;
    options.optimizeSize = false;
    options.validate = false;
    glslang::GlslangToSpv(*program.getIntermediate(stage), spirv, &logger, &options);

    std::cerr << logger.getAllMessages();
    if (spirv.empty()) {
        throw std::runtime_error("No SPIR-V generated");
    }

    WriteDepfile(argv[3], argv[2], includer.dependencies);

    std::ofstream output;
    output.exceptions(std::ios::failbit | std::ios::badbit);
    output.open(argv[2], std::ios::binary);
    output << "// File generated by host shader compiler.\n\n"
              "#pragma once\n\n"
              "#include <cstdint>\n\n"
              "namespace HostShaders {\ninline constexpr std::uint32_t "
           << argv[4] << "[] = {\n    ";

    for (std::size_t i = 0; i < spirv.size(); ++i) {
        output << "0x" << std::hex << spirv[i] << ", ";
    }
    output << "\n};\n} // namespace HostShaders\n";
    output.close();
}

int main(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "Usage: shader_compiler source header depfile symbol [NAME=VALUE ...]\n";
        return 1;
    }

    if (!glslang::InitializeProcess()) {
        std::cerr << "Failed to initialize glslang\n";
        return 1;
    }

    int result = 0;
    try {
        Compile(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << argv[1] << ": " << error.what() << '\n';
        result = 1;
    }

    glslang::FinalizeProcess();
    return result;
}
