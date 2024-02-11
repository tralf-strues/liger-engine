/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file default_log.cpp
 * @date 2023-09-10
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
#include <liger/core/log/default_log.hpp>

namespace liger::default_log {

const ConsoleLogWriter::Style kDefaultConsoleLogStyle {
    .default_style = {},

    .write_level = true,
    .use_level_style_for_entire_message = true,

    .level_styles = {
      {LogLevel::kInfo,    {}},
      {LogLevel::kTrace,   fg(fmt::color::olive_drab)},
      {LogLevel::kWarning, fg(fmt::color::medium_orchid)},
      {LogLevel::kError,   fg(fmt::color::red)},
      {LogLevel::kFatal,   fmt::emphasis::bold | fg(fmt::color::red)}
    },

    .level_names = {
      {LogLevel::kInfo,    "INFO"},
      {LogLevel::kTrace,   "TRACE"},
      {LogLevel::kWarning, "WARN"},
      {LogLevel::kError,   "ERROR"},
      {LogLevel::kFatal,   "FATAL"}
    },

    .write_channel = true
};

Log CreateDefaultLog() {
  Log log;
  log.AddWriter(std::make_unique<ConsoleLogWriter>(kDefaultConsoleLogStyle));

  return log;
}

Log g_Log = CreateDefaultLog();

}  // namespace liger::default_log