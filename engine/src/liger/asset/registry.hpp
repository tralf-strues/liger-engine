/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file registry.hpp
 * @date 2024-03-10
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

#pragma once

#include <liger/asset/id.hpp>

#include <filesystem>
#include <unordered_map>
#include <unordered_set>

namespace liger::asset {

/**
 * @brief Registry of assets contained in an asset folder.
 *
 * Manages mapping from asset uuids to their physical file paths.
 * All this information is also gets saved to the corresponding registry file.
 *
 * Example structure of an asset folder:
 *     assets/
 *         .lregistry
 *         textures/
 *             player/
 *                 player_albedo.png
 *                 player_normal.png
 *         materials/
 *             player.lmat
 *         meshes/
 *             player.lmesh
 *         scenes/
 *             scene0.lscene
 *         sounds/
 *             player_hello.mp3
 *             player_goodbye.mp3
 *
 * Example contents of the corresponding `.liger-registry` file:
 *     - file: textures/player/player_albedo.png
 *       id: 0x7449545984958451
 *     - file: textures/player/player_normal.png
 *       id: 0x2435204985724523
 *     ...
 *     - file: materials/player.lmat
 *       id: 0x9208347234895237
 *     ...
 *     - file: meshes/player.lmesh
 *       id: 0x9045734534058964
 *     ...
 *     - file: scenes/scene0.lscene
 *       id: 0x1894576549867059
 *     ...
 *     - file: sounds/player_hello.mp3
 *       id: 0x5924984576345097
 *     - file: sounds/player_goodbye.mp3
 *       id: 0x2489524375902435
 *     ...
 */
class Registry {
 public:
  /** @brief Open and load the registry. */
  explicit Registry(std::filesystem::path registry_file);

  /** @brief Save the registry if valid. */
  ~Registry();

  Registry(const Registry& other)            = delete;
  Registry& operator=(const Registry& other) = delete;

  Registry(Registry&& other)            = default;
  Registry& operator=(Registry&& other) = default;

  /**
   * @brief Whether the registry is valid (i.e. upon construction file was found and successfully loaded).
   */
  explicit operator bool() const;

  /**
   * @brief Whether the registry is valid (i.e. upon construction file was found and successfully loaded).
   */
  bool Valid() const;

  /**
   * @brief Save the registry to file.
   * @return Whether save was successful.
   */
  bool Save() const;

  /**
   * @brief Whether the registry contains an asset with this id.
   */
  bool Contains(Id id) const;

  /**
   * @brief Get the relative filepath of the asset not including the asset folder.
   */
  const std::filesystem::path& GetRelativeFile(Id id) const;

  /**
   * @brief Get the relative filepath of the asset including the asset folder.
   */
  std::filesystem::path GetAbsoluteFile(Id id) const;

  /**
   * @brief Register a new asset with the specified file.
   */
  Id Register(std::filesystem::path file);

  /**
   * @brief Update the filepath corresponding to the registered asset.
   */
  void UpdateFile(Id id, std::filesystem::path new_file);

  /**
   * @brief Remove the asset from the registry.
   *
   * @warning The method does not guarantee the validity of the registry after
   *          this operation, as there can appear hanging dependencies in
   *          case the unregistered asset was a dependency to other assets.
   */
  void Unregister(Id id);

 private:
  bool ReadRegistryFile();

  bool                                          valid_{false};
  std::filesystem::path                         asset_folder_;
  std::filesystem::path                         registry_file_;
  std::unordered_map<Id, std::filesystem::path> files_;
};

}  // namespace liger::asset