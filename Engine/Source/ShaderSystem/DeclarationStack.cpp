/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file DeclarationStack.cpp
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

#include <Liger-Engine/ShaderSystem/DeclarationStack.hpp>

#include <Liger-Engine/Core/EnumReflection.hpp>
#include <Liger-Engine/ShaderSystem/LogChannel.hpp>

namespace liger::shader {

[[nodiscard]] bool MergeDataBlocks(Declaration& dst, const Declaration& src) {
  dst.data_block += "\n\n";
  dst.data_block += src.data_block;

  return true;
}

[[nodiscard]] bool MergeMemberLists(std::vector<Declaration::Member>&       dst,
                                    const std::vector<Declaration::Member>& src) {
  for (const auto& src_member : src) {
    auto existing_member = std::find_if(dst.begin(), dst.end(), [&src_member](const auto& dst_member) {
      return dst_member.name == src_member.name;
    });

    if (existing_member == dst.end()) {
      dst.emplace_back(src_member);
      continue;
    }

    if (existing_member->type != src_member.type) {
      LIGER_LOG_ERROR(kLogChannelShader, "Merge member collision (name = {}, src_type = {}, dst_type = {})",
                      src_member.name, EnumToString(src_member.type), EnumToString(existing_member->type));
      return false;
    }

    // TODO (tralf-strues): add other merge member checks.
  }

  return true;
}

[[nodiscard]] bool MergeCodeSnippets(std::vector<Declaration::CodeSnippet>&       dst,
                                     const std::vector<Declaration::CodeSnippet>& src) {
  for (const auto& src_snippet : src) {
    auto existing_snippet = std::find_if(dst.begin(), dst.end(), [&src_snippet](const auto& dst_snippet) {
      return dst_snippet.name == src_snippet.name;
    });

    if (src_snippet.name.empty() || existing_snippet == dst.end()) {
      dst.emplace_back(src_snippet);
      continue;
    }

    if (existing_snippet->code != src_snippet.code) {
      LIGER_LOG_ERROR(kLogChannelShader,
                      "Merge code snippet collision (name = {}): \n"
                      "Src code snippet:\n{}"
                      "Dst code snippet:\n{}",
                      src_snippet.name, src_snippet.code, existing_snippet->code);
      return false;
    }

    if (existing_snippet->insert != src_snippet.insert) {
      LIGER_LOG_ERROR(kLogChannelShader,
                      "Merge code snippet insert policy collision (name = {}, src_policy = {}, dst_policy = {})",
                      src_snippet.name, EnumToString(src_snippet.insert), EnumToString(existing_snippet->insert));
      return false;
    }
  }

  return true;
}

// [[nodiscard]] bool MergeSockets(std::vector<Declaration::Socket>& dst, const std::vector<Declaration::Socket>& src) {
//   for (const auto& src_socket : src) {
//     auto existing_socket = std::find_if(dst.begin(), dst.end(), [&src_socket](const auto& dst_socket) {
//       return dst_socket.name == src_socket.name;
//     });

//     if (existing_socket == dst.end()) {
//       dst.emplace_back(src_socket);
//       continue;
//     }
//   }

//   return true;
// }

[[nodiscard]] bool MergeCode(Declaration& dst, const Declaration& src) {
  if (src.scope == Declaration::Scope::None && (!src.code.empty() || !dst.code.empty())) {
    LIGER_LOG_ERROR(kLogChannelShader, "Merge code error, Common scope cannot contain code blocks!");
    return false;
  }

  if (src.scope == Declaration::Scope::None) {
    return true;
  }

  if (!src.code.empty() && !dst.code.empty()) {
    LIGER_LOG_ERROR(kLogChannelShader,
                    "Merge code error, only one code block per shader type can be declared! Scope: {}",
                    EnumToString(src.scope));
    return false;
  }

  dst.code += src.code;

  return true;
}

[[nodiscard]] bool MergePipelineDescription(Declaration& dst, const Declaration& src) {
  if (src.vertex_topology) {
    dst.vertex_topology = src.vertex_topology;
  }

  if (src.rasterization) {
    dst.rasterization = src.rasterization;
  }

  if (src.depth_stencil_test) {
    dst.depth_stencil_test = src.depth_stencil_test;
  }

  if (src.color_blend) {
    dst.color_blend = src.color_blend;
  }

  if (src.attachments) {
    dst.attachments= src.attachments;
  }

  if (src.thread_group_size) {
    dst.thread_group_size = src.thread_group_size;
  }

  return true;
}

[[nodiscard]] bool Merge(Declaration& dst, const Declaration& src) {
  if (!MergeDataBlocks(dst, src)) {
    return false;
  }

  if (!MergeMemberLists(dst.input, src.input)) {
    return false;
  }

  if (!MergeMemberLists(dst.output, src.output)) {
    return false;
  }

  if (!MergeCodeSnippets(dst.code_snippets, src.code_snippets)) {
    return false;
  }

  // if (!MergeSockets(dst.sockets, src.sockets)) {
  //   return false;
  // }

  if (!MergeCode(dst, src)) {
    return false;
  }

  if (!MergePipelineDescription(dst, src)) {
    return false;
  }

  for (const auto& src_declaration : src.declarations) {
    Declaration* dst_declaration = nullptr;

    bool scope_added = false;
    for (auto& added_declaration : dst.declarations) {
      if (src_declaration.scope == added_declaration.scope) {
        scope_added = true;
        dst_declaration = &added_declaration;
        break;
      }
    }

    if (!scope_added) {
      dst_declaration = &dst.declarations.emplace_back();
      dst_declaration->scope = src_declaration.scope;
    }

    if (!Merge(*dst_declaration, src_declaration)) {
      return false;
    }
  }

  return true;
}

void DeclarationStack::Push(Declaration declaration) {
  stack_.emplace_back(std::move(declaration));
}

void DeclarationStack::Pop() {
  stack_.pop_back();
}

Declaration& DeclarationStack::Top() {
  return stack_.back();
}

std::optional<Declaration> DeclarationStack::Merged() const {
  Declaration merged;
  merged.scope = Declaration::Scope::None;

  for (const auto& src_declaration : stack_) {
    if (!Merge(merged, src_declaration)) {
      return std::nullopt;
    }
  }

  return merged;
}

}  // namespace liger::shader