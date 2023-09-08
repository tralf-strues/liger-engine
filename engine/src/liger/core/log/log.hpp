/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file log.hpp
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

#include "liger/core/log/log_message.hpp"
#include "liger/core/log/log_writer.hpp"

namespace liger {

/**
 * @brief Container of messages, that can have several @ref ILogWriter writers.
 */
class Log {
 public:
  using MessageIterator = std::vector<LogMessage>::const_iterator;

 public:
  void AddWriter(std::unique_ptr<ILogWriter> writer);

  template <typename... Args>
  void Message(LogLevel level, uint64 channel, Args&&... args);

  MessageIterator begin() const;
  MessageIterator end() const;

 private:
  void OnMessageAdded();

 private:
  std::vector<LogMessage> messages_;
  std::vector<std::unique_ptr<ILogWriter>> writers_;
};

}  // namespace liger

#define LOG_IMPL
#include "liger/core/log/log.ipp"
#undef LOG_IMPL