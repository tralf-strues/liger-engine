/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file ShaderLoader.cpp
 * @date 2024-05-07
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

#include <Liger-Engine/Asset/Loaders/ShaderLoader.hpp>

#include <Liger-Engine/Asset/Manager.hpp>
#include <Liger-Engine/ShaderSystem/DeclarationParser.hpp>
#include <Liger-Engine/ShaderSystem/Shader.hpp>

namespace liger::asset::loaders {

ShaderLoader::ShaderLoader(rhi::IDevice& device) : compiler_(device) {}

const std::filesystem::path& ShaderLoader::FileExtension() const {
  static std::filesystem::path extension{".lshader"};
  return extension;
}

void ShaderLoader::Load(asset::Manager& manager, asset::Id asset_id, const std::filesystem::path& filepath) {
  auto shader = manager.GetAsset<shader::Shader>(asset_id);

  shader::DeclarationParser parser(filepath);
  if (!parser.Valid()) {
    shader.UpdateState(asset::State::Invalid);
    return;
  }

  auto declaration = parser.Parse();
  if (!declaration) {
    shader.UpdateState(asset::State::Invalid);
    return;
  }

  if (!compiler_.Compile(*shader, declaration.value(), filepath.stem().string())) {
    shader.UpdateState(asset::State::Invalid);
    return;
  }

  shader.UpdateState(asset::State::Loaded);
}

}  // namespace liger::asset::loaders