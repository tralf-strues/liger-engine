/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Importer.hpp
 * @date 2024-03-14
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

#include <filesystem>
#include <string_view>

namespace liger::asset {

/**
 * @brief Importer interface.
 *
 * Importers are basically converters of one asset file (based on extension) to possibly several
 * asset files better suited for some particular usage. An example can be an FBX importer that
 * generates mesh/material/texture/etc. files in a format which is tailored specifically for a
 * particular renderer implementation.
 */
class IImporter {
 public:
  /**
   * @brief Import result which (if successful) lists imported files and dependencies between them.
   */
  struct Result {
    /**
     * @brief Whether import has finished successfully.
     */
    bool success{false};

    /**
     * @brief List of filepaths of imported assets.
     *
     * @note The filepaths are based on the import destination folder. For instance, if `dst_folder`
     *       is "assets/imported/", and the importer generated file "teapot.mesh", then the filepath
     *       will be "assets/imported/teapot.mesh".
     */
    std::vector<std::filesystem::path> files;

    /**
     * @brief List of dependencies between assets.
     *
     * Each dependency is represented by two indices into @ref files list. The first index is the
     * index of the dependent file and the second one is of its dependency file.
     */
    std::vector<std::pair<uint32_t, uint32_t>> dependencies;
  };

  virtual ~IImporter() = default;

  /**
   * @brief File extension of files which this importer can import, e.g. ('.fbx').
   */
  virtual const std::filesystem::path& FileExtension() const = 0;

  /**
   * @brief Try to import `src` file and save generated files to `dst_folder`.
   */
  virtual Result Import(const std::filesystem::path& src, const std::filesystem::path& dst_folder) const = 0;
};

}  // namespace liger::asset