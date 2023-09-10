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

#include "liger/core/log/console_log_writer.hpp"
#include "liger/core/log/log_channels.hpp"
#include "liger/core/log/log_level.hpp"

using namespace liger;

ConsoleLogWriter::ConsoleLogWriter(const ConsoleLogWriter::Style& style) : style_(style) {}

void ConsoleLogWriter::SetStyle(const ConsoleLogWriter::Style& style) { style_ = style; }
const ConsoleLogWriter::Style& ConsoleLogWriter::GetStyle() const { return style_; }

void ConsoleLogWriter::OnMessageAdded(const LogMessage& message) {
  if (style_.write_level) {
    fmt::print(GetLevelStyle(message.level), "{0} ", GetLevelName(message.level));
  }

  if (style_.write_channel) {
    fmt::print(GetTextStyle(message.level), "{0} ", GetChannelName(message.channel));
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

cstring ConsoleLogWriter::GetLevelName(LogLevel level) const {
  auto it = style_.level_names.find(level);
  return (it != style_.level_names.end()) ? it->second.c_str() : "";
}

cstring ConsoleLogWriter::GetChannelName(uint64 channel) const {
  auto it = style_.channel_names.find(channel);
  return (it != style_.channel_names.end()) ? it->second.c_str() : "";
}

const liger::ConsoleLogWriter::Style liger::kDefaultConsoleLogStyle = {
  /*default_style=*/{},

  /*write_level=*/true,
  /*use_level_style_for_entire_message=*/true,
  /*level_styles=*/
  {{LogLevel::kInfo,    {}},
   {LogLevel::kTrace,   fg(fmt::color::olive_drab)},
   {LogLevel::kWarning, fg(fmt::color::purple)},
   {LogLevel::kError,   fg(fmt::color::red)},
   {LogLevel::kFatal,   fmt::emphasis::bold | fg(fmt::color::red)}},
  /*level_names=*/
  {{LogLevel::kInfo,    "[INFO] "},
   {LogLevel::kTrace,   "[TRACE]"},
   {LogLevel::kWarning, "[WARN] "},
   {LogLevel::kError,   "[ERROR]"},
   {LogLevel::kFatal,   "[FATAL]"}},
  
  /*write_channel=*/true,
  /*channel_names=*/
  {{(uint64_t)LogChannel::kNone,    "[None]   "},
   {(uint64_t)LogChannel::kCore,    "[Core]   "},
   {(uint64_t)LogChannel::kRender,  "[Render] "},
   {(uint64_t)LogChannel::kECS,     "[ECS]    "},
   {(uint64_t)LogChannel::kPhysics, "[Physics]"}}
};
