/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Compiler.cpp
 * @date 2024-04-17
 *
 * The MIT License (MIT)
 * Copyright (c) 2023 Nikita Mochalov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <Liger-Engine/ShaderSystem/Compiler.hpp>

#include <Liger-Engine/Core/EnumReflection.hpp>
#include <Liger-Engine/ShaderSystem/LogChannel.hpp>

#include <fmt/ostream.h>
#include <shaderc/shaderc.hpp>

#include <memory>
#include <set>
#include <sstream>
#include <unordered_set>

namespace liger::shader {

const char * const kSourceHeader =
#include "ShaderSourceHeader.glsl"
;

enum class PipelineType : uint32_t {
  Invalid,
  Graphics,
  Compute
};

/************************************************************************************************
 * Utility functions
 ************************************************************************************************/
inline const char* ToString(Declaration::Member::BufferLayout layout) {
  switch (layout) {
    case Declaration::Member::BufferLayout::Std140: { return "std140"; }
    case Declaration::Member::BufferLayout::Std430: { return "std430"; }
  }

  return nullptr;
}

inline const char* ToString(Declaration::Member::BufferAccess access) {
  switch (access) {
    case Declaration::Member::BufferAccess::ReadOnly:  { return "readonly";  }
    case Declaration::Member::BufferAccess::WriteOnly: { return "writeonly"; }
    case Declaration::Member::BufferAccess::ReadWrite: { return "";          }
  }

  return nullptr;
}

inline bool IsResourceType(Declaration::Member::Type type) {
  return (type == Declaration::Member::Type::UniformBuffer) || (type == Declaration::Member::Type::StorageBuffer) ||
         (type == Declaration::Member::Type::Sampler2D) || (type == Declaration::Member::Type::Sampler2DArray);
}

inline const char* ToString(Declaration::Member::Type type) {
  switch (type) {
    case Declaration::Member::Type::Bool:    { return "bool";      }
    case Declaration::Member::Type::Int32:   { return "int32_t";   }
    case Declaration::Member::Type::UInt32:  { return "uint32_t";  }
    case Declaration::Member::Type::Float32: { return "float32_t"; }
    case Declaration::Member::Type::F32Vec2: { return "f32vec2";   }
    case Declaration::Member::Type::F32Vec3: { return "f32vec3";   }
    case Declaration::Member::Type::F32Vec4: { return "f32vec4";   }
    case Declaration::Member::Type::F32Mat3: { return "f32mat3";   }
    case Declaration::Member::Type::F32Mat4: { return "f32mat4";   }

    default: { return nullptr; }
  }
}

[[nodiscard]] inline std::string ToSnakeCase(std::string_view name) {
  if (name.empty()) {
    return "";
  }

  std::stringstream ss;
  ss << static_cast<char>(std::tolower(name[0]));

  for (const auto ch : name.substr(1)) {
    if (std::isupper(ch)) {
      ss << '_';
    }

    ss << static_cast<char>(std::tolower(ch));
  }

  return ss.str();
}

[[nodiscard]] inline shaderc_shader_kind ToShadercKind(Declaration::Scope scope) {
  switch (scope) {
    case Declaration::Scope::Vertex:   { return shaderc_glsl_vertex_shader;   }
    case Declaration::Scope::Fragment: { return shaderc_glsl_fragment_shader; }
    case Declaration::Scope::Compute:  { return shaderc_glsl_compute_shader;  }

    default: {
      LIGER_LOG_FATAL(kLogChannelShader, "Unsupported scope {0}", EnumToString(scope));
      return shaderc_glsl_compute_shader;
    }
  }
}

[[nodiscard]] inline uint32_t TypeSize(Declaration::Member::Type type) {
  switch (type) {
    case Declaration::Member::Type::Bool:    { return         sizeof(uint32_t); }
    case Declaration::Member::Type::Int32:   { return         sizeof(int32_t);  }
    case Declaration::Member::Type::UInt32:  { return         sizeof(uint32_t); }
    case Declaration::Member::Type::Float32: { return         sizeof(float);    }
    case Declaration::Member::Type::F32Vec2: { return     2 * sizeof(float);    }
    case Declaration::Member::Type::F32Vec3: { return     3 * sizeof(float);    }
    case Declaration::Member::Type::F32Vec4: { return     4 * sizeof(float);    }
    case Declaration::Member::Type::F32Mat3: { return 3 * 3 * sizeof(float);    }
    case Declaration::Member::Type::F32Mat4: { return 4 * 4 * sizeof(float);    }

    default: { return 0; }
  }
}

/************************************************************************************************
 * Stage linking
 ************************************************************************************************/
struct StageLinking {
  struct Member {
    std::string_view          name;
    Declaration::Member::Type type;
  };

  using MemberList = std::vector<Member>;

  MemberList vertex_to_fragment;
  MemberList fragment_out;
};

[[nodiscard]] PipelineType DeterminePipelineType(const Declaration& declaration) {
  Declaration::Scope scopes;

  for (const auto& module_declaration : declaration.declarations) {
    if (module_declaration.scope != Declaration::Scope::None) {
      scopes |= module_declaration.scope;
    }
  }

  bool graphics = EnumBitmaskContains(scopes, Declaration::Scope::Vertex | Declaration::Scope::Fragment);
  bool compute  = EnumBitmaskContains(scopes, Declaration::Scope::Compute);

  if (graphics) {
    return PipelineType::Graphics;
  }

  if (compute) {
    return PipelineType::Compute;
  }

  LIGER_LOG_ERROR(kLogChannelShader, "Declaration contains neither complete graphics nor compute pipeline shaders.");
  return PipelineType::Invalid;
}

[[nodiscard]] std::optional<StageLinking> LinkGraphicsStages(const Declaration& declaration) {
  StageLinking linking;

  const Declaration* vert{nullptr};
  const Declaration* frag{nullptr};

  for (const auto& stage_declaration : declaration.declarations) {
    if (stage_declaration.scope == Declaration::Scope::Vertex) {
      vert = &stage_declaration;
    }

    if (stage_declaration.scope == Declaration::Scope::Fragment) {
      frag = &stage_declaration;
    }
  }

  if (!vert || !frag) {
    return std::nullopt;
  }

  /* Vertex to fragment IO */
  for (const auto& vert_out : vert->output) {
    if (vert_out.modifier != Declaration::Member::Modifier::StageIO) {
      continue;
    };

    bool found = false;
    for (const auto& frag_in : frag->input) {
      if (vert_out.name == frag_in.name) {
        found = true;
        break;
      }
    }

    if (!found) {
      LIGER_LOG_ERROR(kLogChannelShader, "Vertex output member '{0}' has no corresponding fragment input member.");
      return std::nullopt;
    }

    linking.vertex_to_fragment.emplace_back(StageLinking::Member{.name = vert_out.name, .type = vert_out.type});
  }

  /* Fragment output */
  for (const auto& frag_out : frag->output) {
    if (frag_out.modifier == Declaration::Member::Modifier::StageIO) {
      linking.fragment_out.emplace_back(StageLinking::Member{.name = frag_out.name, .type = frag_out.type});
    }
  }

  return linking;
}

/************************************************************************************************
 * Common data gather
 ************************************************************************************************/
struct PushConstantMember {
  std::string               name;
  Declaration::Member::Type type;

  bool operator<(const PushConstantMember& rhs) const {
    return name < rhs.name;
  }
};

struct PushConstantMembers {
  Declaration::Scope           scopes_mask;
  std::set<PushConstantMember> members;
};

PushConstantMembers GatherPushConstants(const Declaration& declaration) {
  PushConstantMembers push_constants;

  bool globals_found = true;
  auto add = [&push_constants, &globals_found](const Declaration& declaration, const Declaration::Member& member) {
    if (IsResourceType(member.type)) {
      push_constants.members.insert(PushConstantMember {
        .name = "binding_" + ToSnakeCase(member.name),
        .type = Declaration::Member::Type::UInt32
      });

      push_constants.scopes_mask |= declaration.scope;
      globals_found = globals_found || (declaration.scope == rhi::IShaderModule::Type::None);
    } else if (member.modifier == Declaration::Member::Modifier::PushConstant) {
      push_constants.members.insert(PushConstantMember {
        .name = member.name,
        .type = member.type
      });

      push_constants.scopes_mask |= declaration.scope;
      globals_found = globals_found || (declaration.scope == rhi::IShaderModule::Type::None);
    }
  };

  for (const auto& member : declaration.input)  { add(declaration, member); }
  for (const auto& member : declaration.output) { add(declaration, member); }

  for (const auto& sub_declaration : declaration.declarations) {
    if (globals_found) {
      push_constants.scopes_mask |= sub_declaration.scope;
    }

    for (const auto& member : sub_declaration.input)  { add(sub_declaration, member); }
    for (const auto& member : sub_declaration.output) { add(sub_declaration, member); }
  }

  uint32_t total_size = 0U;
  for (const auto& member : push_constants.members) {
    total_size += TypeSize(member.type);
  }

  uint32_t pad_bytes = (16U - (total_size % 16)) % 16;
  for (uint32_t i = 0; i < pad_bytes / 4U; ++i) {
    push_constants.members.insert(PushConstantMember {
      .name = fmt::format("pad_{0}", i),
      .type = Declaration::Member::Type::UInt32
    });
  }

  return push_constants;
}

/************************************************************************************************
 * Source generation
 ************************************************************************************************/
void RegisterBuffer(std::stringstream& source, const Declaration::Member& member) {
  switch (member.type) {
    case Declaration::Member::Type::UniformBuffer: {
      fmt::print(source,
                 "RegisterUniformBuffer({0}, {{\n"
                 "  {1}\n"
                 "}});\n",
                 member.name, member.buffer_contents);
      break;
    }

    case Declaration::Member::Type::StorageBuffer: {
      fmt::print(source,
                 "RegisterStorageBuffer({0}, {1}, {2}, {{\n"
                 "  {3}\n"
                 "}});\n",
                 ToString(member.buffer_layout), ToString(member.buffer_access), member.name, member.buffer_contents);
      break;
    }

    default: {}
  }
}

void DeclarePushConstant(std::stringstream& source, const PushConstantMembers& push_constants) {
  fmt::print(source, "layout(push_constant) uniform PushConstant {{\n");

  for (const auto& member : push_constants.members) {
    fmt::print(source, "  {0} {1};\n", ToString(member.type), member.name);
  }

  fmt::print(source, "}} push_constant;\n");
}

void DeclareFragmentShaderInput(std::stringstream& source, StageLinking::MemberList& members) {
  fmt::print(source, "layout(location = 0) in _FragmentInput_ {{\n");

  for (const auto& member : members) {
    fmt::print(source, "  {0} {1};\n", ToString(member.type), member.name);
  }

  fmt::print(source, "}} fragment_input;\n");
}

void DeclareStageOutput(std::stringstream& source, StageLinking::MemberList& members) {
  if (members.empty()) {
    return;
  }

  for (uint32_t i = 0; i < members.size(); ++i) {
    fmt::print(source, "layout(location = {0}) out {1} {2};\n", i, ToString(members[i].type), members[i].name);
  }
}

void DeclareInputStruct(std::stringstream& source, const Declaration& common, const Declaration& shader) {
  fmt::print(source, "struct LigerInput {{\n");

  auto add_member = [&source](const auto& member) {
    if (IsResourceType(member.type)) {
      fmt::print(source, "  uint32_t binding_{0};\n", ToSnakeCase(member.name));
    } else if (member.type == Declaration::Member::Type::VertexIndex ||
               member.type == Declaration::Member::Type::InstanceIndex) {
      fmt::print(source, "  uint32_t {0};\n", member.name);
    } else {
      fmt::print(source, "  {0} {1};\n", ToString(member.type), member.name);
    }
  };

  for (const auto& member : common.input) {
    add_member(member);
  }

  for (const auto& member : shader.input) {
    add_member(member);
  }

  fmt::print(source, "}};\n");
}

void DeclareGlobalCode(std::stringstream& source, const Declaration& common, const Declaration& shader) {
  for (const auto& snippet : common.code_snippets) {
    if (snippet.insert == Declaration::CodeSnippet::InsertPolicy::Global) {
      source << snippet.code << '\n';
    }
  }

  for (const auto& snippet : shader.code_snippets) {
    if (snippet.insert == Declaration::CodeSnippet::InsertPolicy::Global) {
      source << snippet.code << '\n';
    }
  }
}

void DeclareInputFill(std::stringstream& source, const Declaration& common, const Declaration& shader) {
  fmt::print(source, "LigerInput liger_in;\n");

  auto fill_member = [&source, &shader](const auto& member) {
    if (IsResourceType(member.type)) {
      fmt::print(source, "liger_in.binding_{0} = push_constant.binding_{0};\n", ToSnakeCase(member.name));
    } else if (member.type == Declaration::Member::Type::VertexIndex) {
      fmt::print(source, "liger_in.{0} = gl_VertexIndex;\n", member.name);
    } else if (member.type == Declaration::Member::Type::InstanceIndex) {
      fmt::print(source, "liger_in.{0} = gl_InstanceIndex;\n", member.name);
    } else if (member.modifier == Declaration::Member::Modifier::PushConstant) {
      fmt::print(source, "liger_in.{0} = push_constant.{0};\n", member.name);
    } else if (member.modifier == Declaration::Member::Modifier::StageIO &&
               shader.scope == Declaration::Scope::Fragment) {
      fmt::print(source, "liger_in.{0} = fragment_input.{0};\n", member.name);
    }
  };

  for (const auto& member : common.input) {
    fill_member(member);
  }

  for (const auto& member : shader.input) {
    fill_member(member);
  }

  source << '\n';
}

void DeclareLocalCode(std::stringstream& source, const Declaration& common, const Declaration& shader) {
  for (const auto& snippet : common.code_snippets) {
    if (snippet.insert == Declaration::CodeSnippet::InsertPolicy::Local) {
      source << snippet.code << '\n';
    }
  }

  for (const auto& snippet : shader.code_snippets) {
    if (snippet.insert == Declaration::CodeSnippet::InsertPolicy::Local) {
      source << snippet.code << '\n';
    }
  }

  source << '\n';
}

bool DeclareMainFunction(std::stringstream& source, const Declaration& common, const Declaration& shader) {
  if (shader.scope == Declaration::Scope::Compute) {
    if (!shader.thread_group_size) {
      LIGER_LOG_ERROR(kLogChannelShader, "Thread group size is not specified for compute shaders");
      return false;
    }

    fmt::print(source, "layout(local_size_x = {0}, local_size_y = {1}, local_size_z = {2}) in;\n",
               shader.thread_group_size.value()[0U], shader.thread_group_size.value()[1U],
               shader.thread_group_size.value()[2U]);
  }

  fmt::print(source, "void main() {{\n");

  DeclareInputFill(source, common, shader);
  DeclareLocalCode(source, common, shader);
  source << shader.code << '\n';

  fmt::print(source, "}}\n");

  return true;
}

[[nodiscard]] std::optional<std::string> GenerateSource(const Declaration& common, const Declaration& shader,
                                                        std::optional<StageLinking>& stage_linking,
                                                        const PushConstantMembers& push_constants) {
  std::stringstream source;
  source << kSourceHeader;

  auto blank_line = [&source]() { source << '\n'; };

  source << common.data_block;
  blank_line();
  source << shader.data_block;
  blank_line();

  for (const auto& member : common.input) {
    RegisterBuffer(source, member);
    blank_line();
  }

  for (const auto& member : shader.input) {
    RegisterBuffer(source, member);
    blank_line();
  }

  if (EnumBitmaskContains(push_constants.scopes_mask, shader.scope)) {
    DeclarePushConstant(source, push_constants);
    blank_line();
  }

  if (shader.scope == Declaration::Scope::Vertex) {
    DeclareStageOutput(source, stage_linking->vertex_to_fragment);
    blank_line();
  } else if (shader.scope == Declaration::Scope::Fragment) {
    DeclareFragmentShaderInput(source, stage_linking->vertex_to_fragment);
    DeclareStageOutput(source, stage_linking->fragment_out);
    blank_line();
  }

  DeclareInputStruct(source, common, shader);
  blank_line();

  DeclareGlobalCode(source, common, shader);
  blank_line();

  if (!DeclareMainFunction(source, common, shader)) {
    return std::nullopt;
  }

  return source.str();
}

std::vector<uint32_t> CompileToBinary(Declaration::Scope scope, const char* source) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;

  auto preprocessor_result = compiler.PreprocessGlsl(source, ToShadercKind(scope), "main", options);
  if (preprocessor_result.GetCompilationStatus() != shaderc_compilation_status_success) {
    LIGER_LOG_ERROR(kLogChannelShader, "Preprocessing failed:\n{0}", preprocessor_result.GetErrorMessage());
    return {};
  }

  std::string preprocessed_source = {preprocessor_result.cbegin(), preprocessor_result.cend()};

  // LIGER_LOG_INFO(kLogChannelShader, "Preprocessed shader source (stage = {0}):\n{1}", EnumToString(scope),
  //                preprocessed_source);

  // options.AddMacroDefinition("MY_DEFINE", "1"); // TODO (tralf-strues): Add macro substitutions support
  // options.SetOptimizationLevel(shaderc_optimization_level_performance);
  options.SetGenerateDebugInfo();
  options.SetVulkanRulesRelaxed(true);
  options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
  options.SetTargetSpirv(shaderc_spirv_version_1_5);

  auto compiler_result = compiler.CompileGlslToSpv(preprocessed_source, ToShadercKind(scope), "main", options);
  if (compiler_result.GetCompilationStatus() != shaderc_compilation_status_success) {
    LIGER_LOG_ERROR(kLogChannelShader, "Compiling failed:\n{0}", compiler_result.GetErrorMessage());
    return {};
  }

  return {compiler_result.cbegin(), compiler_result.cend()};
}

Compiler::Compiler(rhi::IDevice& device) : device_(device) {}

bool Compiler::Compile(Shader& shader, const Declaration& declaration) {
  if (declaration.scope != Declaration::Scope::None) {
    LIGER_LOG_ERROR(kLogChannelShader, "Declaration's scope must be Common, instead it is {}",
                    EnumToString(declaration.scope));
    return false;
  }

  auto pipeline_type = DeterminePipelineType(declaration);
  if (pipeline_type == PipelineType::Invalid) {
    return false;
  }

  auto stage_linking  = LinkGraphicsStages(declaration);
  auto push_constants = GatherPushConstants(declaration);

  std::unordered_map<Declaration::Scope, std::string> sources;
  for (const auto& shader_declaration : declaration.declarations) {
    auto source = GenerateSource(declaration, shader_declaration, stage_linking, push_constants);
    if (!source) {
      return false;
    }

    sources[shader_declaration.scope] = source.value();
  }

  std::vector<std::unique_ptr<rhi::IShaderModule>> shader_modules;
  for (const auto& [scope, source] : sources) {
    // LIGER_LOG_INFO(kLogChannelShader, "Generated shader source (stage = {0}):\n{1}", EnumToString(scope), source);

    auto binary = CompileToBinary(scope, source.c_str());
    if (binary.empty()) {
      return false;
    }

    auto shader_module = device_.CreateShaderModule({.type = scope, .source_binary = binary});
    if (!shader_module) {
      return false;
    }

    shader_modules.emplace_back(std::move(shader_module));
  }

  for (const auto& member : push_constants.members) {
    shader.push_constant_offsets_[member.name] = shader.push_constant_size_;
    shader.push_constant_size_ += TypeSize(member.type);
  }

  shader.push_constant_data_ = new char[shader.push_constant_size_];

  if (pipeline_type == PipelineType::Graphics) {
    std::vector<const rhi::IShaderModule*> shader_module_refs;
    shader_module_refs.reserve(shader_modules.size());

    for (const auto& shader_module : shader_modules) {
      shader_module_refs.push_back(shader_module.get());
    }

    rhi::IPipeline::GraphicsInfo pipeline_info {
      .input_assembly     = {.topology = declaration.vertex_topology.value()},
      .rasterization      = declaration.rasterization.value(),
      .depth_stencil_test = declaration.depth_stencil_test.value(),
      .blend              = declaration.color_blend.value(),
      .push_constant      = {.size = shader.push_constant_size_, .shader_types = push_constants.scopes_mask},
      .attachments        = declaration.attachments.value(),
      .shader_modules     = shader_module_refs
    };

    shader.pipeline_ = device_.CreatePipeline(pipeline_info);
    if (!shader.pipeline_) {
      return false;
    }
  } else if (pipeline_type == PipelineType::Compute) {
    rhi::IPipeline::ComputeInfo pipeline_info {
      .push_constant = {.size = shader.push_constant_size_, .shader_types = push_constants.scopes_mask},
      .shader_module = shader_modules[0].get()
    };

    shader.pipeline_ = device_.CreatePipeline(pipeline_info);
    if (!shader.pipeline_) {
      return false;
    }
  }

  return true;
}

}  // namespace liger::shader