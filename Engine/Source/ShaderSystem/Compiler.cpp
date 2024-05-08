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
// #include <shaderc/shaderc.hpp>

// #include <glslang/Public/ResourceLimits.h>
// #include <glslang/Public/ShaderLang.h>
// #include <SPIRV/GlslangToSpv.h>
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>

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

inline bool IsBufferType(Declaration::Member::Type type) {
  return (type == Declaration::Member::Type::UniformBuffer) || (type == Declaration::Member::Type::StorageBuffer);
}

inline bool IsTextureType(Declaration::Member::Type type) {
  return (type == Declaration::Member::Type::Sampler2D) || (type == Declaration::Member::Type::Sampler2DArray);
}

inline bool IsResourceType(Declaration::Member::Type type) {
  return IsBufferType(type) || IsTextureType(type);
}

inline const char* ToString(Declaration::Member::Type type) {
  switch (type) {
    case Declaration::Member::Type::Bool:    { return "bool";      }
    case Declaration::Member::Type::Int32:   { return "int32_t";   }
    case Declaration::Member::Type::UInt32:  { return "uint32_t";  }
    case Declaration::Member::Type::UInt64:  { return "uint64_t";  }
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

// [[nodiscard]] inline shaderc_shader_kind ToShadercKind(Declaration::Scope scope) {
//   switch (scope) {
//     case Declaration::Scope::Vertex:   { return shaderc_glsl_vertex_shader;   }
//     case Declaration::Scope::Fragment: { return shaderc_glsl_fragment_shader; }
//     case Declaration::Scope::Compute:  { return shaderc_glsl_compute_shader;  }

//     default: {
//       LIGER_LOG_FATAL(kLogChannelShader, "Unsupported scope {0}", EnumToString(scope));
//       return shaderc_glsl_compute_shader;
//     }
//   }
// }

[[nodiscard]] inline glslang_stage_t ToGLSLangShaderStage(Declaration::Scope scope) {
  switch (scope) {
    case Declaration::Scope::Vertex:   { return GLSLANG_STAGE_VERTEX;   }
    case Declaration::Scope::Fragment: { return GLSLANG_STAGE_FRAGMENT; }
    case Declaration::Scope::Compute:  { return GLSLANG_STAGE_COMPUTE;  }

    default: {
      LIGER_LOG_FATAL(kLogChannelShader, "Unsupported scope {0}", EnumToString(scope));
      return GLSLANG_STAGE_VERTEX;
    }
  }
}

[[nodiscard]] inline uint32_t TypeSize(Declaration::Member::Type type) {
  switch (type) {
    case Declaration::Member::Type::UniformBuffer:
    case Declaration::Member::Type::StorageBuffer: { return         sizeof(uint64_t); }

    case Declaration::Member::Type::Bool:          { return         sizeof(uint32_t); }
    case Declaration::Member::Type::Int32:         { return         sizeof(int32_t);  }
    case Declaration::Member::Type::UInt32:        { return         sizeof(uint32_t); }
    case Declaration::Member::Type::UInt64:        { return         sizeof(uint64_t); }
    case Declaration::Member::Type::Float32:       { return         sizeof(float);    }
    case Declaration::Member::Type::F32Vec2:       { return     2 * sizeof(float);    }
    case Declaration::Member::Type::F32Vec3:       { return     3 * sizeof(float);    }
    case Declaration::Member::Type::F32Vec4:       { return     4 * sizeof(float);    }
    case Declaration::Member::Type::F32Mat3:       { return 3 * 3 * sizeof(float);    }
    case Declaration::Member::Type::F32Mat4:       { return 4 * 4 * sizeof(float);    }

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
  std::string               buffer_name;
  Declaration::Member::Type type;
  uint32_t                  offset;

  bool operator<(const PushConstantMember& rhs) const {
    return name < rhs.name;
  }
};

struct PushConstantMembers {
  Declaration::Scope              scopes_mask;
  std::vector<PushConstantMember> members;
  uint32_t                        size;
};

PushConstantMembers GatherPushConstants(const Declaration& declaration) {
  PushConstantMembers push_constants;

  bool globals_found = true;
  auto add = [&push_constants, &globals_found](const Declaration& declaration, const Declaration::Member& member) {
    if (IsTextureType(member.type)) {
      push_constants.members.emplace_back(PushConstantMember {
        .name = "binding_" + ToSnakeCase(member.name),
        .type = Declaration::Member::Type::UInt32
      });

      push_constants.scopes_mask |= declaration.scope;
      globals_found = globals_found || (declaration.scope == rhi::IShaderModule::Type::None);
    } else if (IsBufferType(member.type)) {
      push_constants.members.emplace_back(PushConstantMember {
        .name        = ToSnakeCase(member.name),
        .buffer_name = member.name,
        .type        = member.type
      });

      push_constants.scopes_mask |= declaration.scope;
      globals_found = globals_found || (declaration.scope == rhi::IShaderModule::Type::None);
    } else if (member.modifier == Declaration::Member::Modifier::PushConstant) {
      push_constants.members.emplace_back(PushConstantMember {
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
  for (auto& member : push_constants.members) {
    member.offset = total_size;

    if (IsBufferType(member.type)) {
      member.offset += (8U - (total_size % 8U)) % 8U;
    }

    total_size = member.offset + TypeSize(member.type);
  }

  uint32_t pad_bytes = (16U - (total_size % 16)) % 16;
  for (uint32_t i = 0; i < pad_bytes / 4U; ++i) {
    push_constants.members.emplace_back(PushConstantMember {
      .name = fmt::format("pad_{0}", i),
      .type = Declaration::Member::Type::UInt32
    });

    total_size += TypeSize(Declaration::Member::Type::UInt32);
  }

  push_constants.size = total_size;

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
  fmt::print(source, "layout(push_constant, scalar) uniform PushConstant {{\n");

  for (const auto& member : push_constants.members) {
    if (IsBufferType(member.type)) {
      fmt::print(source, "  {0} {1};\n", member.buffer_name, member.name);
    } else {
      fmt::print(source, "  {0} {1};\n", ToString(member.type), member.name);
    }
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
    if (IsTextureType(member.type)) {
      fmt::print(source, "  uint32_t binding_{0};\n", ToSnakeCase(member.name));
    } else if (IsBufferType(member.type)) {
      fmt::print(source, "  {0} {1};\n", member.name, ToSnakeCase(member.name));
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
    if (IsTextureType(member.type)) {
      fmt::print(source, "liger_in.binding_{0} = push_constant.binding_{0};\n", ToSnakeCase(member.name));
    } else if (IsBufferType(member.type)) {
      fmt::print(source, "liger_in.{0} = push_constant.{0};\n", ToSnakeCase(member.name));
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
  source = R"=====(
#version 450

#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable

layout(set = 0, binding = 0) uniform sampler2D global_samplers_2d[];
layout(r32f, set = 0, binding = 1) writeonly uniform image2D global_images_2d[];

struct ParticleSystem {
  uint32_t max_particles;
  float32_t spawn_rate;
  float32_t lifetime;

  f32vec3 velocity_min;
  f32vec3 velocity_max;

  f32vec4 color_start;
  f32vec4 color_end;

  float32_t size_start;
  float32_t size_end;
};

struct Particle {
  f32vec3 position;
  f32vec3 velocity;
  f32vec4 color;
  float32_t size;
  float32_t lifetime;
};

layout(buffer_reference, buffer_reference_align=64) readonly buffer EmitterData { ParticleSystem emitter; };

layout(push_constant, scalar) uniform PushConstant {
  // Particles particles;
  // FreeList free_list;
  uint32_t particles_to_spawn;
  float32_t time;
  EmitterData emitter_data;
};

struct LigerInput {
  uint32_t particles_to_spawn;
  float32_t time;
};

float32_t RandomFloat(float32_t seed) {
  return fract(sin(seed * (91.3458f)) * 47453.5453f);
}

float32_t RandomFloat(f32vec2 seed) {
  return fract(sin(dot(seed.xy, f32vec2(12.9898f, 78.233f))) * 43758.5453123f);
}

f32vec2 RandomVec2(float32_t seed) {
  return f32vec2(RandomFloat(seed), RandomFloat(seed + 5.0932f));
}

f32vec3 RandomVec3(float32_t seed) {
  return f32vec3(RandomFloat(seed), RandomFloat(seed + 5.0932f), RandomFloat(seed + 89.2348f));
}

void EmitParticle(out Particle particle, float32_t random_seed, ParticleSystem emitter) {
  particle.position = f32vec3(0.0f);
  particle.velocity = mix(emitter.velocity_min, emitter.velocity_max, RandomVec3(random_seed));
  particle.lifetime = emitter.lifetime;
}

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
  // LigerInput liger_in;
  // liger_in.emitter_data = push_constant.emitter_data;
  // liger_in.particles = push_constant.particles;
  // liger_in.free_list = push_constant.free_list;
  // liger_in.particles_to_spawn = push_constant.particles_to_spawn;
  // liger_in.time = push_constant.time;

  uint32_t idx = gl_GlobalInvocationID.x;
  if (idx >= particles_to_spawn) {
    return;
  }

  // int32_t free_list_idx = atomicAdd(free_list.count, - 1) - 1;
  // if (free_list_idx < 0) {

  //   atomicAdd(free_list.count, 1);
  //   return;
  // }

  // int32_t particle_idx = free_list.indices[free_list_idx];
  // float32_t random_seed = (float32_t(particle_idx) + float32_t(free_list_idx)) * time;
  ParticleSystem emitter = emitter_data.emitter;

  // EmitParticle(particles.particles[particle_idx], random_seed, emitter);
}
  )=====";

  glslang_initialize_process();

  const glslang_input_t input = {
      .language = GLSLANG_SOURCE_GLSL,
      .stage = ToGLSLangShaderStage(scope),
      .client = GLSLANG_CLIENT_VULKAN,
      .client_version = GLSLANG_TARGET_VULKAN_1_3,
      .target_language = GLSLANG_TARGET_SPV,
      .target_language_version = GLSLANG_TARGET_SPV_1_6,
      .code = source,
      .default_version = 100,
      .default_profile = GLSLANG_NO_PROFILE,
      .force_default_version_and_profile = 0,
      .forward_compatible = 0,
      .messages = GLSLANG_MSG_DEFAULT_BIT,
      .resource = glslang_default_resource(),
  };

  glslang_shader_t* shader = glslang_shader_create(&input);

  if (!glslang_shader_preprocess(shader, &input)) {
    LIGER_LOG_ERROR(kLogChannelShader, "Shader preprocessing failed (scope = {0}):\n{1}\n{2}", EnumToString(scope),
                    glslang_shader_get_info_log(shader), glslang_shader_get_info_debug_log(shader));
    return {};
  }

  if (!glslang_shader_parse(shader, &input)) {
    LIGER_LOG_ERROR(kLogChannelShader, "Shader parsing failed (scope = {0}):\n{1}\n{2}", EnumToString(scope),
                    glslang_shader_get_info_log(shader), glslang_shader_get_info_debug_log(shader));
    return {};
  }

  glslang_program_t* program = glslang_program_create();
  glslang_program_add_shader(program, shader);

  if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
    LIGER_LOG_ERROR(kLogChannelShader, "Shader linking failed (scope = {0}):\n{1}\n{2}", EnumToString(scope),
                    glslang_program_get_info_log(program), glslang_program_get_info_debug_log(program));
    return {};
  }

  glslang_spv_options_t options = {
    .generate_debug_info                  = true,
    .strip_debug_info                     = false,
    .disable_optimizer                    = true,
    .optimize_size                        = false,
    .disassemble                          = false,
    .validate                             = true,
    .emit_nonsemantic_shader_debug_info   = false,
    .emit_nonsemantic_shader_debug_source = false,
  };

  glslang_program_SPIRV_generate_with_options(program, input.stage, &options);

  if (glslang_program_SPIRV_get_messages(program)) {
    LIGER_LOG_INFO(kLogChannelShader, "SPIRV message:\n{0}", glslang_program_SPIRV_get_messages(program));
  }

  // const VkShaderModuleCreateInfo ci = {
  //     .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
  //     .codeSize = glslang_program_SPIRV_get_size(program) * sizeof(uint32_t),
  //     .pCode = glslang_program_SPIRV_get_ptr(program),
  // };

  std::vector<uint32_t> binary;
  binary.resize(glslang_program_SPIRV_get_size(program));
  std::memcpy(binary.data(), glslang_program_SPIRV_get_ptr(program), binary.size() * sizeof(uint32_t));

  glslang_program_delete(program);
  glslang_shader_delete(shader);

  glslang_finalize_process();

  return binary;

  // glslang::InitializeProcess();
  // auto sh_language = ToGLSLangShaderStage(scope);

  // glslang::TShader shader(sh_language);
  // shader.setEnvInput(glslang::EShSourceGlsl, sh_language, glslang::EShClientVulkan, 100);
  // shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
  // shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
  // shader.setStrings(&source, 1U);

  // std::string preprocessed_source;

  // glslang::TShader::ForbidIncluder includer{};
  // if (!shader.preprocess(GetDefaultResources(), 450, ENoProfile, false, false, EShMsgDefault, &preprocessed_source, includer)) {
  //   LIGER_LOG_ERROR(kLogChannelShader, "Preprocessing failed:\n{0}\n{1}\nCode:\n{2}", shader.getInfoLog(),
  //                   shader.getInfoDebugLog(), preprocessed_source);
  //   return {};
  // }

  // // LIGER_LOG_INFO(kLogChannelShader, "Preprocessed source (stage = {0}):\n{1}", EnumToString(scope), preprocessed_source);

  // if (!shader.parse(GetDefaultResources(), 450, false, EShMsgDefault)) {
  //   LIGER_LOG_ERROR(kLogChannelShader, "Parsing failed:\n{0}\n{1}", shader.getInfoLog(), shader.getInfoDebugLog());
  //   return {};
  // }

  // glslang::TProgram program;
  // program.addShader(&shader);
  // if (!program.link(static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules))) {
  //   LIGER_LOG_ERROR(kLogChannelShader, "Linking failed:\n{0}\n{1}", shader.getInfoLog(), shader.getInfoDebugLog());
  //   return {};
  // }

  // // Convert the intermediate generated by glslang to SPIR-V
  // glslang::TIntermediate& intermediate = *(program.getIntermediate(sh_language));
  // std::vector<uint32_t> spirv;

  // glslang::SpvOptions options{};
  // options.validate = true;

  // glslang::GlslangToSpv(intermediate, spirv, &options);

  // glslang::FinalizeProcess();

  // return spirv;






//   shaderc::Compiler compiler;
//   shaderc::CompileOptions options;

//   auto preprocessor_result = compiler.PreprocessGlsl(source, ToShadercKind(scope), "main", options);
//   if (preprocessor_result.GetCompilationStatus() != shaderc_compilation_status_success) {
//     LIGER_LOG_ERROR(kLogChannelShader, "Preprocessing failed:\n{0}", preprocessor_result.GetErrorMessage());
//     return {};
//   }

//   std::string preprocessed_source = {preprocessor_result.cbegin(), preprocessor_result.cend()};

//   // options.AddMacroDefinition("MY_DEFINE", "1"); // TODO (tralf-strues): Add macro substitutions support
//   options.SetOptimizationLevel(shaderc_optimization_level_performance);

//   options.SetGenerateDebugInfo();
//   options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
//   options.SetTargetSpirv(shaderc_spirv_version_1_5);
//   options.SetVulkanRulesRelaxed(true);

//   preprocessed_source = R"=====(
// #version 450

// #extension GL_EXT_shader_explicit_arithmetic_types : require
// #extension GL_EXT_nonuniform_qualifier : enable
// #extension GL_EXT_buffer_reference : enable
// #extension GL_EXT_scalar_block_layout : enable

// layout(set = 0, binding = 0) uniform sampler2D global_samplers_2d[];
// layout(r32f, set = 0, binding = 1) writeonly uniform image2D global_images_2d[];

// struct ParticleSystem {
//   uint32_t max_particles;
//   float32_t spawn_rate;
//   float32_t lifetime;

//   f32vec3 velocity_min;
//   f32vec3 velocity_max;

//   f32vec4 color_start;
//   f32vec4 color_end;

//   float32_t size_start;
//   float32_t size_end;
// };

// struct Particle {
//   f32vec3 position;
//   f32vec3 velocity;
//   f32vec4 color;
//   float32_t size;
//   float32_t lifetime;
// };

// layout(buffer_reference, scalar) readonly buffer EmitterData { ParticleSystem emitter; };
// layout(buffer_reference, scalar) writeonly buffer Particles { Particle particles[]; };
// layout(buffer_reference, scalar) buffer FreeList { int32_t count; int32_t indices[]; };

// layout(push_constant, scalar) uniform PushConstant {
//   EmitterData emitter_data;
//   Particles particles;
//   FreeList free_list;
//   uint32_t particles_to_spawn;
//   float32_t time;
// };

// struct LigerInput {
//   EmitterData emitter_data;
//   Particles particles;
//   FreeList free_list;
//   uint32_t particles_to_spawn;
//   float32_t time;
// };

// float32_t RandomFloat(float32_t seed) {
//   return fract(sin(seed * (91.3458f)) * 47453.5453f);
// }

// float32_t RandomFloat(f32vec2 seed) {
//   return fract(sin(dot(seed.xy, f32vec2(12.9898f, 78.233f))) * 43758.5453123f);
// }

// f32vec2 RandomVec2(float32_t seed) {
//   return f32vec2(RandomFloat(seed), RandomFloat(seed + 5.0932f));
// }

// f32vec3 RandomVec3(float32_t seed) {
//   return f32vec3(RandomFloat(seed), RandomFloat(seed + 5.0932f), RandomFloat(seed + 89.2348f));
// }

// void EmitParticle(out Particle particle, float32_t random_seed, ParticleSystem emitter) {
//   particle.position = f32vec3(0.0f);
//   particle.velocity = mix(emitter.velocity_min, emitter.velocity_max, RandomVec3(random_seed));
//   particle.lifetime = emitter.lifetime;
// }

// layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
// void main() {
//   // LigerInput liger_in;
//   // liger_in.emitter_data = push_constant.emitter_data;
//   // liger_in.particles = push_constant.particles;
//   // liger_in.free_list = push_constant.free_list;
//   // liger_in.particles_to_spawn = push_constant.particles_to_spawn;
//   // liger_in.time = push_constant.time;

//   uint32_t idx = gl_GlobalInvocationID.x;
//   if (idx >= particles_to_spawn) {
//     return;
//   }

//   int32_t free_list_idx = atomicAdd(free_list.count, - 1) - 1;
//   if (free_list_idx < 0) {

//     atomicAdd(free_list.count, 1);
//     return;
//   }

//   int32_t particle_idx = free_list.indices[free_list_idx];
//   float32_t random_seed = (float32_t(particle_idx) + float32_t(free_list_idx)) * time;
//   ParticleSystem emitter = emitter_data.emitter;

//   EmitParticle(particles.particles[particle_idx], random_seed, emitter);
// }
//   )=====";

//   LIGER_LOG_INFO(kLogChannelShader, "Preprocessed shader source (stage = {0}):\n{1}", EnumToString(scope),
//                  preprocessed_source);

//   auto compiler_result = compiler.CompileGlslToSpv(preprocessed_source, ToShadercKind(scope), "main", options);
//   if (compiler_result.GetCompilationStatus() != shaderc_compilation_status_success) {
//     LIGER_LOG_ERROR(kLogChannelShader, "Compiling failed:\n{0}\n\nPreprocessed shader source (stage = {1}):\n{2}",
//                     compiler_result.GetErrorMessage(), EnumToString(scope), preprocessed_source);
//     return {};
//   }

//   return {compiler_result.cbegin(), compiler_result.cend()};
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
    shader.push_constant_offsets_[member.name] = member.offset;
  }

  shader.push_constant_size_ = push_constants.size;
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

  LIGER_LOG_INFO(kLogChannelShader, "Successfully compiled the shader");

  return true;
}

}  // namespace liger::shader