/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file console_log_writer.cpp
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

#include <liger/core/log/console_log_writer.hpp>

namespace liger {

ConsoleLogWriter::ConsoleLogWriter(Style style) : style_(std::move(style)) {}

void ConsoleLogWriter::SetStyle(const ConsoleLogWriter::Style& style) { style_ = style; }
const ConsoleLogWriter::Style& ConsoleLogWriter::GetStyle() const { return style_; }

void ConsoleLogWriter::OnMessageAdded(const LogMessage& message) {
  if (style_.write_level) {
    fmt::print(GetTextStyle(message.level) | fmt::emphasis::bold, "[");
    fmt::print(GetLevelStyle(message.level) | fmt::emphasis::bold, "{0}", GetLevelName(message.level));
    fmt::print(GetTextStyle(message.level) | fmt::emphasis::bold, "]");
  }

  if (style_.write_source && !message.source.empty()) {
    fmt::print(GetTextStyle(message.level), "[");
    fmt::print(GetLevelStyle(message.level) | fmt::emphasis::underline, "{0}", message.source);
    fmt::print(GetTextStyle(message.level), "]");
  }

  if (style_.write_channel && !message.channel.empty()) {
    fmt::print(GetTextStyle(message.level), "[{0}] ", message.channel);
  }

  fmt::print(GetTextStyle(message.level), "{0}\n", message.message);
}

const fmt::text_style& ConsoleLogWriter::GetTextStyle(LogLevel level) const {
  return style_.use_level_style_for_entire_message ? GetLevelStyle(level) : style_.default_style;
}

const fmt::text_style& ConsoleLogWriter::GetLevelStyle(LogLevel level) const {
  auto it = style_.level_styles.find(level);
  return (it != style_.level_styles.end()) ? it->second : style_.default_style;
}

const char* ConsoleLogWriter::GetLevelName(LogLevel level) const {
  auto it = style_.level_names.find(level);
  return (it != style_.level_names.end()) ? it->second.c_str() : "";
}

}  // namespace liger