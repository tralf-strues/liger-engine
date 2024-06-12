/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file DeclarationParser.cpp
 * @date 2024-04-27
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

#include <Liger-Engine/ShaderSystem/DeclarationParser.hpp>

#include <Liger-Engine/Core/EnumReflection.hpp>
#include <Liger-Engine/ShaderSystem/DeclarationStack.hpp>
#include <Liger-Engine/ShaderSystem/LogChannel.hpp>

#include <unordered_map>

namespace liger::shader {

static const std::unordered_map<std::string, Declaration::Member::Type> kStringToMemberType {
  {"uniform-buffer",   Declaration::Member::Type::UniformBuffer},
  {"storage-buffer",   Declaration::Member::Type::StorageBuffer},

  {"sampler2D",        Declaration::Member::Type::Sampler2D},
  {"sampler2DArray",   Declaration::Member::Type::Sampler2DArray},
  {"storage-texture",  Declaration::Member::Type::StorageTexture},

  {"bool",             Declaration::Member::Type::Bool},
  {"int32_t",          Declaration::Member::Type::Int32},
  {"uint32_t",         Declaration::Member::Type::UInt32},
  {"float32_t",        Declaration::Member::Type::Float32},

  {"u32vec2",          Declaration::Member::Type::U32Vec2},
  {"u32vec3",          Declaration::Member::Type::U32Vec3},
  {"u32vec4",          Declaration::Member::Type::U32Vec4},

  {"f32vec2",          Declaration::Member::Type::F32Vec2},
  {"f32vec3",          Declaration::Member::Type::F32Vec3},
  {"f32vec4",          Declaration::Member::Type::F32Vec4},

  {"f32mat3",          Declaration::Member::Type::F32Mat3},
  {"f32mat4",          Declaration::Member::Type::F32Mat4},

  {"vertex-index",     Declaration::Member::Type::VertexIndex},
  {"instance-index",   Declaration::Member::Type::InstanceIndex}
};

static const std::unordered_map<std::string, Declaration::Member::BufferLayout> kStringToMemberBufferLayout {
  {"std140",           Declaration::Member::BufferLayout::Std140},
  {"std430",           Declaration::Member::BufferLayout::Std430}
};

static const std::unordered_map<std::string, Declaration::Member::BufferAccess> kStringToMemberBufferAccess {
  {"readonly",         Declaration::Member::BufferAccess::ReadOnly},
  {"writeonly",        Declaration::Member::BufferAccess::WriteOnly},
  {"readwrite",        Declaration::Member::BufferAccess::ReadWrite}
};

static const std::unordered_map<std::string, Declaration::Member::Modifier> kStringToMemberModifier {
  {"property",         Declaration::Member::Modifier::Property},
  {"compile-constant", Declaration::Member::Modifier::CompileConstant},
  {"push-constant",    Declaration::Member::Modifier::PushConstant}
};

static const std::unordered_map<std::string, Declaration::CodeSnippet::InsertPolicy> kStringToSnippetInsertPolicy {
  {"auto-global",      Declaration::CodeSnippet::InsertPolicy::Global},
  {"auto-local",       Declaration::CodeSnippet::InsertPolicy::Local}
};

static const std::unordered_map<std::string, Declaration::Scope> kStringToScope {
  {"VertexShader",     Declaration::Scope::Vertex},
  {"FragmentShader",   Declaration::Scope::Fragment},
  {"ComputeShader",    Declaration::Scope::Compute}
};

bool IsBuiltInStageVariableType(Declaration::Member::Type type) {
  return type == Declaration::Member::Type::VertexIndex || type == Declaration::Member::Type::InstanceIndex;
}

bool ParseDataBlock(Declaration& declaration, YAML::Node& root) {
  if (auto data_node = root["Data"]) {
    declaration.data_block = data_node.as<std::string>();
  }

  return true;
}

bool ParseIOMember(Declaration::Member& member, YAML::Node& member_node, bool is_shader_scope) {
  if (auto name_node = member_node["Name"]) {
    member.name = name_node.as<std::string>();
  } else {
    LIGER_LOG_ERROR(kLogChannelShader, "Member node does not contain 'Name' property");
    return false;
  }

  if (auto type_node = member_node["Type"]) {
    auto type_str = type_node.as<std::string>();
    auto type_it  = kStringToMemberType.find(type_str);
    if (type_it != kStringToMemberType.end()) {
      member.type = type_it->second;
    } else {
      LIGER_LOG_ERROR(kLogChannelShader, "Property 'Type' contains unknown token '{0}'", type_str);
      return false;
    }
  } else {
    LIGER_LOG_ERROR(kLogChannelShader, "Member '{0}' does not contain 'Type' property", member.name);
    return false;
  }

  if (auto layout_node = member_node["Layout"]) {
    auto layout_str = layout_node.as<std::string>();
    auto layout_it  = kStringToMemberBufferLayout.find(layout_str);
    if (layout_it != kStringToMemberBufferLayout.end()) {
      member.buffer_layout = layout_it->second;
    } else {
      LIGER_LOG_ERROR(kLogChannelShader, "Property 'Layout' contains unknown token '{0}'", layout_str);
      return false;
    }
  } else if (member.type == Declaration::Member::Type::StorageBuffer) {
    LIGER_LOG_ERROR(kLogChannelShader,
                    "Member '{0}' does not contain 'Access' property, which is required for storage buffer members",
                    member.name);
    return false;
  }

  if (auto access_node = member_node["Access"]) {
    auto access_str = access_node.as<std::string>();
    auto access_it  = kStringToMemberBufferAccess.find(access_str);
    if (access_it != kStringToMemberBufferAccess.end()) {
      member.buffer_access = access_it->second;
    } else {
      LIGER_LOG_ERROR(kLogChannelShader, "Property 'Access' contains unknown token '{0}'", access_str);
      return false;
    }
  } else if (member.type == Declaration::Member::Type::StorageBuffer) {
    LIGER_LOG_ERROR(kLogChannelShader,
                    "Member '{0}' does not contain 'Access' property, which is required for storage buffer members",
                    member.name);
    return false;
  }

  if (auto contents_node = member_node["Contents"]) {
    member.buffer_contents = contents_node.as<std::string>();
  } else if (IsBufferType(member.type)) {
    LIGER_LOG_ERROR(kLogChannelShader,
                    "Member '{0}' does not contain 'Contents' property, which is required for buffer members",
                    member.name);
    return false;
  }

  if (auto modifier_node = member_node["Modifier"]) {
    auto modifier_str = modifier_node.as<std::string>();
    auto modifier_it  = kStringToMemberModifier.find(modifier_str);
    if (modifier_it != kStringToMemberModifier.end()) {
      member.modifier = modifier_it->second;
    } else {
      LIGER_LOG_ERROR(kLogChannelShader, "Property 'Modifier' contains unknown token '{0}'", modifier_str);
      return false;
    }
  } else if (is_shader_scope && !IsResourceType(member.type)) {
    member.modifier = Declaration::Member::Modifier::StageIO;
  } else {
    member.modifier = Declaration::Member::Modifier::Property;
  }

  return true;
}

bool ParseIO(Declaration& declaration, YAML::Node& root) {
  bool is_shader_scope = declaration.scope != Declaration::Scope::None;

  if (auto input_node = root["Input"]) {
    declaration.input.resize(input_node.size());

    for (size_t i = 0; i < input_node.size(); ++i) {
      auto member_node = input_node[i];
      if (!ParseIOMember(declaration.input[i], member_node, is_shader_scope)) {
        return false;
      }
    }
  }

  if (auto output_node = root["Output"]) {
    declaration.output.resize(output_node.size());

    for (size_t i = 0; i < output_node.size(); ++i) {
      auto member_node = output_node[i];
      if (!ParseIOMember(declaration.output[i], member_node, is_shader_scope)) {
        return false;
      }
    }
  }

  return true;
}

bool ParseCodeSnippets(Declaration& declaration, YAML::Node& root) {
  auto code_snippets_node = root["CodeSnippets"];
  if (!code_snippets_node) {
    return true;
  }

  declaration.code_snippets.resize(code_snippets_node.size());
  for (size_t i = 0; i < code_snippets_node.size(); ++i) {
    auto snippet_node = code_snippets_node[i];

    if (auto name_node = snippet_node["Name"]) {
      declaration.code_snippets[i].name = name_node.as<std::string>();
    }

    if (auto insert_node  = snippet_node["Insert"]) {
      auto insert_str = insert_node.as<std::string>();
      auto insert_it  = kStringToSnippetInsertPolicy.find(insert_str);
      if (insert_it != kStringToSnippetInsertPolicy.end()) {
        declaration.code_snippets[i].insert = insert_it->second;
      } else {
        LIGER_LOG_ERROR(kLogChannelShader, "Property 'Insert' contains unknown token '{0}'", insert_str);
        return false;
      }
    } else {
      declaration.code_snippets[i].insert = Declaration::CodeSnippet::InsertPolicy::Global;
    }

    if (auto code_node = snippet_node["Code"]) {
      declaration.code_snippets[i].code = code_node.as<std::string>();
    } else {
      LIGER_LOG_ERROR(kLogChannelShader, "Code snippet does not contain 'Code' property");
      return false;
    }
  }

  return true;
}

bool ParseCode(Declaration& declaration, YAML::Node& root) {
  bool is_shader_scope = declaration.scope != Declaration::Scope::None;

  if (auto code_node = root["Code"]) {
    if (!is_shader_scope) {
      LIGER_LOG_ERROR(kLogChannelShader, "'Code' block can only be used inside a shader scope");
      return false;
    }

    declaration.code = code_node.as<std::string>();
  }

  return true;
}

bool ParseUse(Declaration& declaration, YAML::Node& root) {
  auto use_node = root["Use"];
  if (!use_node) {
    return true;
  }

  for (auto node : use_node) {
    if (auto include_node = node["Include"]) {
      declaration.includes.push_back(include_node.as<std::string>());
    } else if (auto interface_node = node["Interface"]) {
      declaration.interfaces.push_back(interface_node.as<std::string>());
    }
  }

  return true;
}

bool ParseGraphicsPipelineInfo(Declaration& declaration, YAML::Node& root) {
  auto parse_enum = []<typename EnumT>(const YAML::Node& parent_node, const char* name, EnumT& out_value) -> bool {
    if (auto enum_node = parent_node[name]) {
      auto value_str = enum_node.as<std::string>();
      if (auto value = StringToEnum<EnumT>(value_str)) {
        out_value = *value;
      } else {
        LIGER_LOG_ERROR(kLogChannelShader, "Property '{0}' contains unknown token '{1}'", name, value_str);
        return false;
      }
    }

    return true;
  };

  if (auto input_assembly_node = root["InputAssemblyInfo"]) {
    declaration.vertex_topology = rhi::InputAssemblyInfo::Topology::TriangleList;

    if (!parse_enum(input_assembly_node, "Topology", declaration.vertex_topology.value())) {
      return false;
    }
  }

  if (auto rasterization_node = root["RasterizationInfo"]) {
    declaration.rasterization = rhi::RasterizationInfo{};

    if (!parse_enum(rasterization_node, "CullMode", declaration.rasterization->cull_mode)) {
      return false;
    }

    if (!parse_enum(rasterization_node, "FrontFace", declaration.rasterization->front_face)) {
      return false;
    }

    if (!parse_enum(rasterization_node, "PolygonMode", declaration.rasterization->polygon_mode)) {
      return false;
    }
  }

  if (auto depth_stencil_test_node = root["DepthStencilTestInfo"]) {
    declaration.depth_stencil_test = rhi::DepthStencilTestInfo{};

    if (auto depth_test_enable_node = depth_stencil_test_node["DepthTestEnable"]) {
      declaration.depth_stencil_test->depth_test_enable = depth_test_enable_node.as<bool>();
    }

    if (auto depth_write_enable_node = depth_stencil_test_node["DepthWriteEnable"]) {
      declaration.depth_stencil_test->depth_write_enable = depth_write_enable_node.as<bool>();
    }

    if (!parse_enum(depth_stencil_test_node, "DepthCompareOperation",
                    declaration.depth_stencil_test->depth_compare_operation)) {
      return false;
    }
  }

  if (auto color_blend_node = root["ColorBlendInfo"]) {
    declaration.color_blend = rhi::ColorBlendInfo{};

    if (auto enable_node = color_blend_node["Enable"]) {
      declaration.color_blend->enable = enable_node.as<bool>();
    }

    if (!parse_enum(color_blend_node, "SrcColorFactor", declaration.color_blend->src_color_factor)) {
      return false;
    }

    if (!parse_enum(color_blend_node, "DstColorFactor", declaration.color_blend->dst_color_factor)) {
      return false;
    }

    if (!parse_enum(color_blend_node, "ColorOperation", declaration.color_blend->color_operation)) {
      return false;
    }

    if (!parse_enum(color_blend_node, "SrcAlphaFactor", declaration.color_blend->src_alpha_factor)) {
      return false;
    }

    if (!parse_enum(color_blend_node, "DstAlphaFactor", declaration.color_blend->dst_alpha_factor)) {
      return false;
    }

    if (!parse_enum(color_blend_node, "AlphaOperation", declaration.color_blend->alpha_operation)) {
      return false;
    }
  }

  if (auto attachments_node = root["AttachmentInfo"]) {
    declaration.attachments = rhi::AttachmentInfo{};

    if (auto render_targets_node = attachments_node["RenderTargets"]) {
      declaration.attachments->color_target_formats.reserve(render_targets_node.size());

      for (auto target_node : render_targets_node) {
        auto value_str = target_node.as<std::string>();
        if (auto value = StringToEnum<rhi::Format>(value_str)) {
          declaration.attachments->color_target_formats.push_back(*value);
        } else {
          LIGER_LOG_ERROR(kLogChannelShader, "Property '{0}' contains unknown token '{1}'", "RenderTargets", value_str);
          return false;
        }
      }
    }

    if (!parse_enum(attachments_node, "DepthStencilTarget", declaration.attachments->depth_stencil_format)) {
      return false;
    }
  }

  return true;
}

bool ParseComputePipelineInfo(Declaration& declaration, YAML::Node& root) {
  if (auto thread_group_size_node = root["ThreadGroupSize"]) {
    if (thread_group_size_node.size() < 1 || thread_group_size_node.size() > 3) {
      LIGER_LOG_ERROR(kLogChannelShader, "`ThreadGroupSize` must contain 1 to 3 integer numbers");
      return false;
    }

    declaration.thread_group_size = {1, 1, 1};
    for (uint32_t i = 0; i < thread_group_size_node.size(); ++i) {
      declaration.thread_group_size.value()[i] = thread_group_size_node[i].as<uint32_t>();
    }
  }

  return true;
}

bool ParseDeclaration(Declaration& declaration, YAML::Node& root) {
  if (!ParseDataBlock(declaration, root)) {
    return false;
  }

  if (!ParseIO(declaration, root)) {
    return false;
  }

  if (!ParseCodeSnippets(declaration, root)) {
    return false;
  }

  if (!ParseCode(declaration, root)) {
    return false;
  }

  if (!ParseUse(declaration, root)) {
    return false;
  }

  if (!ParseGraphicsPipelineInfo(declaration, root)) {
    return false;
  }

  if (!ParseComputePipelineInfo(declaration, root)) {
    return false;
  }

  return true;
}

DeclarationParser::DeclarationParser(std::filesystem::path filepath) : filepath_(std::move(filepath)) {
  root_node_ = YAML::LoadFile(filepath_.string());
}

bool DeclarationParser::Valid() const { return root_node_.IsDefined(); }

std::optional<Declaration> DeclarationParser::Parse() {
  if (!Valid()) {
    return std::nullopt;
  }

  Declaration declaration;
  declaration.scope = Declaration::Scope::None;
  if (!ParseDeclaration(declaration, root_node_)) {
    return std::nullopt;
  }

  for (const auto& [stage_str, scope] : kStringToScope) {
    if (auto sub_declaration_node = root_node_[stage_str]) {
      auto& sub_declaration = declaration.declarations.emplace_back(Declaration{.scope = scope});
      if (!ParseDeclaration(sub_declaration, sub_declaration_node)) {
        return std::nullopt;
      }
    }
  }

  DeclarationStack stack;
  auto dir = filepath_.parent_path();

  auto add_include = [&dir, &stack](std::string_view include_name) -> bool {
    auto include_filepath = dir / std::filesystem::path(include_name);

    DeclarationParser include_parser(include_filepath);
    if (!include_parser.Valid()) {
      return false;
    }

    auto include_decl = include_parser.Parse();
    if (include_decl) {
      stack.Push(std::move(include_decl.value()));
    }

    return include_decl.has_value();
  };

  for (const auto& include_name : declaration.includes) {
    if (!add_include(include_name)) {
      LIGER_LOG_ERROR(kLogChannelShader, "Failed to include '{0}'", include_name);
      return std::nullopt;
    }
  }

  for (const auto& sub_declaration : declaration.declarations) {
    for (const auto& include_name : sub_declaration.includes) {
      if (!add_include(include_name)) {
        LIGER_LOG_ERROR(kLogChannelShader, "Failed to include '{0}'", include_name);
        return std::nullopt;
      }
    }
  }

  stack.Push(std::move(declaration));

  return stack.Merged();
}

}  // namespace liger::shader