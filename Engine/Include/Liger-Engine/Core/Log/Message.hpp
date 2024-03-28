/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Message.hpp
 * @date 2023-09-05
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

#include <cstdint>
#include <string>
#include <string_view>

namespace liger {

enum class LogLevel : uint32_t {
  Info,
  Trace,
  Warning,
  Error,
  Fatal
};

struct LogMessage {
  /** @brief Source of the message (e.g. source file/line, etc.). */
  std::string source;

  /** @brief Describes the importance of the message. */
  LogLevel level{LogLevel::Info};

  /** @brief Allows user-driven channels support (used e.g. for filtering messages). */
  std::string channel;

  /** @brief Message string itself. */
  std::string message;

  LogMessage() = default;

  explicit LogMessage(LogLevel level, std::string_view source, std::string_view channel, std::string_view message);
  explicit LogMessage(LogLevel level, std::string&& source, std::string&& channel, std::string&& message);
};

}  // namespace liger
