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
#include <glslang/Public/ShaderLang.h>

#include <sstream>
#include <unordered_set>
#include "glslang/Public/ResourceLimits.h"

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
}

inline const char* ToString(Declaration::Member::BufferAccess access) {
  switch (access) {
    case Declaration::Member::BufferAccess::ReadOnly:  { return "readonly";  }
    case Declaration::Member::BufferAccess::WriteOnly: { return "writeonly"; }
    case Declaration::Member::BufferAccess::ReadWrite: { return "";          }
  }
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

// [[nodiscard]] inline glslang_stage_t ToGLSLangShaderStage(Declaration::Scope scope) {
//   switch (scope) {
//     case Declaration::Scope::VertexShader:   { return GLSLANG_STAGE_VERTEX;   }
//     case Declaration::Scope::FragmentShader: { return GLSLANG_STAGE_FRAGMENT; }

//     default: {
//       LIGER_LOG_FATAL(kLogChannelShader, "Unsupported scope {0}", EnumToString(scope));
//       return GLSLANG_STAGE_COUNT;
//     }
//   }
// }
[[nodiscard]] inline EShLanguage ToGLSLangShaderStage(Declaration::Scope scope) {
  switch (scope) {
    case Declaration::Scope::VertexShader:   { return EShLangVertex;   }
    case Declaration::Scope::FragmentShader: { return EShLangFragment; }

    default: {
      LIGER_LOG_FATAL(kLogChannelShader, "Unsupported scope {0}", EnumToString(scope));
      return EShLangCount;
    }
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
  std::unordered_set<Declaration::Scope> module_present;

  for (const auto& module_declaration : declaration.declarations) {
    if (module_declaration.scope != Declaration::Scope::Common) {
      module_present.insert(module_declaration.scope);
    }
  }

  bool graphics = module_present.contains(Declaration::Scope::VertexShader) &&
                  module_present.contains(Declaration::Scope::FragmentShader);
  bool compute = module_present.contains(Declaration::Scope::ComputeShader);

  if (compute && (module_present.size() > 1)) {
    LIGER_LOG_ERROR(kLogChannelShader, "Declaration contains both graphics and compute shaders");
    return PipelineType::Invalid;
  }

  if (graphics) {
    return PipelineType::Graphics;
  }

  if (compute) {
    return PipelineType::Compute;
  }

  std::stringstream available_shaders_list;
  for (auto scope : module_present) {
    fmt::print(available_shaders_list, "  - {0}\n", EnumToString(scope));
  }

  LIGER_LOG_ERROR(kLogChannelShader,
      "Declaration contains neither complete graphics nor compute pipeline shaders. The list of shaders is:\n{0}",
      available_shaders_list.str());
  return PipelineType::Invalid;
}

[[nodiscard]] std::optional<StageLinking> LinkGraphicsStages(const Declaration& declaration) {
  StageLinking linking;

  const Declaration* vert{nullptr};
  const Declaration* frag{nullptr};

  for (const auto& stage_declaration : declaration.declarations) {
    if (stage_declaration.scope == Declaration::Scope::VertexShader) {
      vert = &stage_declaration;
    }

    if (stage_declaration.scope == Declaration::Scope::FragmentShader) {
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

void DeclarePushConstant(std::stringstream& source, const Declaration& common, const Declaration& shader) {
  fmt::print(source, "layout(push_constant) uniform PushConstant {{\n");

  /* Resource Bindings */
  auto add_binding = [&source](const Declaration::Member& member) {
    if (IsResourceType(member.type)) {
      fmt::print(source, "  uint32_t binding_{0};\n", ToSnakeCase(member.name));
    }
  };

  for (const auto& member : common.input) {
    add_binding(member);
  }

  for (const auto& member : shader.input) {
    add_binding(member);
  }

  /* Other push constants */
  auto add_push_constant = [&source](const Declaration::Member& member) {
    if (member.modifier == Declaration::Member::Modifier::PushConstant) {
      fmt::print(source, "  {0} {1};\n", ToString(member.type), member.name);
    }
  };

  for (const auto& member : common.input) {
    add_push_constant(member);
  }

  for (const auto& member : shader.input) {
    add_push_constant(member);
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

  fmt::print(source, "layout(location = 0) out _StageOutput_ {{\n");

  for (const auto& member : members) {
    fmt::print(source, "  {0} {1};\n", ToString(member.type), member.name);
  }

  fmt::print(source, "}} output;\n");
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
               shader.scope == Declaration::Scope::FragmentShader) {
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
  if (shader.scope == Declaration::Scope::ComputeShader) {
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
                                                        std::optional<StageLinking>& stage_linking) {
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

  DeclarePushConstant(source, common, shader);
  blank_line();

  if (shader.scope == Declaration::Scope::VertexShader) {
    DeclareStageOutput(source, stage_linking->vertex_to_fragment);
    blank_line();
  } else if (shader.scope == Declaration::Scope::FragmentShader) {
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
  glslang::InitializeProcess();

  glslang::TShader shader(ToGLSLangShaderStage(scope));
  shader.setEnvInput(glslang::EShSourceGlsl, ToGLSLangShaderStage(scope), glslang::EShClientVulkan, 100);
  shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
  shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
  shader.setStrings(&source, 1U);

  std::string preprocessed_source;

  glslang::TShader::ForbidIncluder includer{};
  if (!shader.preprocess(GetDefaultResources(), 450, ENoProfile, false, false, EShMsgDefault, &preprocessed_source, includer)) {
    LIGER_LOG_ERROR(kLogChannelShader, "Preprocessing failed:\n{0}\n{1}\nCode:\n{2}", shader.getInfoLog(),
                    shader.getInfoDebugLog(), preprocessed_source);
    return {};
  }

  LIGER_LOG_INFO(kLogChannelShader, "Preprocessed source (stage = {0}):\n{1}", EnumToString(scope), preprocessed_source);

  if (!shader.parse(GetDefaultResources(), 450, false, EShMsgDefault)) {
    LIGER_LOG_ERROR(kLogChannelShader, "Parsing failed:\n{0}\n{1}", shader.getInfoLog(), shader.getInfoDebugLog());
    return {};
  }

  glslang::TProgram program;
  program.addShader(&shader);
  // if (!program.link(static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules))) {}

  glslang::FinalizeProcess();

  return {};

  // const auto stage = ToGLSLangShaderStage(scope);

  // const glslang_input_t glslang_input {
  //   .language = GLSLANG_SOURCE_GLSL,
  //   .stage = stage,
  //   .client = GLSLANG_CLIENT_VULKAN,
  //   .client_version = GLSLANG_TARGET_VULKAN_1_3,
  //   .target_language = GLSLANG_TARGET_SPV,
  //   .target_language_version = GLSLANG_TARGET_SPV_1_3,
  //   .code = source,
  //   .default_version = 450,
  //   .default_profile = GLSLANG_NO_PROFILE,
  //   .force_default_version_and_profile = 0,
  //   .forward_compatible = 0,
  //   .messages = GLSLANG_MSG_DEFAULT_BIT,
  //   .resource = glslang_default_resource(),
  //   .callbacks = {},
  //   .callbacks_ctx = nullptr,
  // };

  // glslang_shader_t* shader = glslang_shader_create(&glslang_input);

  // // if (!glslang_shader_preprocess(shader, &glslang_input)) {
  // //   LIGER_LOG_ERROR(kLogChannelShader, "Preprocessing failed:\n{0}\n{1}\nCode:\n{2}",
  // //                   glslang_shader_get_info_log(shader), glslang_shader_get_info_debug_log(shader), glslang_input.code);
  // //   return {};
  // // }

  // if (!glslang_shader_parse(shader, &glslang_input)) {
  //   LIGER_LOG_ERROR(kLogChannelShader, "Parsing failed:\n{0}\n{1}\nCode:\n{2}", glslang_shader_get_info_log(shader),
  //                   glslang_shader_get_info_debug_log(shader), glslang_shader_get_preprocessed_code(shader));
  //   return {};
  // }

  // glslang_program_t* program = glslang_program_create();
  // glslang_program_add_shader(program, shader);

  // if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
  //   LIGER_LOG_ERROR(kLogChannelShader, "Linking failed:\n{0}\n{1}", glslang_shader_get_info_log(shader),
  //                   glslang_shader_get_info_debug_log(shader));
  //   return {};
  // }

  // glslang_program_SPIRV_generate(program, stage);

  // std::vector<uint32_t> binary;
  // binary.resize(glslang_program_SPIRV_get_size(program));

  // glslang_program_SPIRV_get(program, binary.data());

  // glslang_program_delete(program);
  // glslang_shader_delete(shader);

  // return binary;
}

std::optional<Shader> Compiler::Compile(const Declaration& declaration) {
  if (declaration.scope != Declaration::Scope::Common) {
    LIGER_LOG_ERROR(kLogChannelShader, "Declaration's scope must be Common, instead it is {}",
                    EnumToString(declaration.scope));
    return std::nullopt;
  }

  auto pipeline_type = DeterminePipelineType(declaration);
  // if (pipeline_type == PipelineType::Invalid) {
  //   return std::nullopt;
  // }

  auto stage_linking = LinkGraphicsStages(declaration);

  std::unordered_map<Declaration::Scope, std::string> sources;
  for (const auto& shader : declaration.declarations) {
    auto source = GenerateSource(declaration, shader, stage_linking);
    if (!source) {
      return std::nullopt;
    }

    sources[shader.scope] = source.value();
  }

  for (const auto& [scope, source] : sources) {
    LIGER_LOG_INFO(kLogChannelShader, "Generated shader source (stage = {0}):\n{1}", EnumToString(scope), source);

    auto binary = CompileToBinary(scope, source.c_str());
    // LIGER_LOG_INFO(kLogChannelShader, "Generated SPIR-V (stage = {0}):\n{1}", EnumToString(scope), binary);
  }

  return std::nullopt;
}

}  // namespace liger::shader