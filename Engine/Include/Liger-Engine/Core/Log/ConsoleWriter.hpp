/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file ConsoleWriter.hpp
 * @date 2023-09-06
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

#include <Liger-Engine/Core/Log/Writer.hpp>

#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/ostream.h>

#include <map>

namespace liger {

/**
 * @brief Writes log messages to console and allows custom styles.
 */
class ConsoleLogWriter : public ILogWriter {
 public:
  struct Style {
    fmt::text_style default_style{};
    fmt::text_style source_style{};

    bool write_source{true};

    bool write_level{true};
    bool use_level_style_for_entire_message{true};
    std::map<LogLevel, fmt::text_style> level_styles;
    std::map<LogLevel, std::string> level_names;

    bool write_channel{true};
  };

  explicit ConsoleLogWriter(Style style);
  ~ConsoleLogWriter() override = default;

  void SetStyle(const Style& style);
  const Style& GetStyle() const;

  void OnMessageAdded(const LogMessage& message) override;

 private:
  const fmt::text_style& GetTextStyle(LogLevel level) const;

  const fmt::text_style& GetLevelStyle(LogLevel level) const;
  const char* GetLevelName(LogLevel level) const;

  Style style_;
};

}  // namespace liger