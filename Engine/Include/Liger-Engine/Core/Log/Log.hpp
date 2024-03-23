/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Log.hpp
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

#include <Liger-Engine/Core/Log/Message.hpp>
#include <Liger-Engine/Core/Log/Writer.hpp>

#include <fmt/core.h>

#include <vector>

namespace liger {

/**
 * @brief Container of messages, that can have several @ref IWriter writers.
 */
class Log {
 public:
  using MessageIterator = std::vector<LogMessage>::const_iterator;

  static Log& Instance();

  void AddWriter(std::unique_ptr<ILogWriter> writer);

  template <typename... Args>
  void Add(LogLevel level, std::string_view source, std::string_view channel, std::string_view format, Args&&... args);

  MessageIterator begin() const;
  MessageIterator end() const;

 private:
  Log() = default;

  void OnMessageAdded();

  std::vector<LogMessage>                  messages_;
  std::vector<std::unique_ptr<ILogWriter>> writers_;
};

template <typename... Args>
void Log::Add(LogLevel level, std::string_view source, std::string_view channel, std::string_view format,
              Args&&... args) {
  messages_.emplace_back(level, source, channel, std::move(fmt::format(fmt::runtime(format), args...)));
  OnMessageAdded();
}

}  // namespace liger

#define LIGER_LINE_TO_STR(x) LIGER_TO_STR(x)
#define LIGER_TO_STR(x) #x

#ifdef __FILE_NAME__
#define LIGER_FILE_NAME __FILE_NAME__
#else
#define LIGER_FILE_NAME __FILE__
#endif

#define LIGER_LOG_INFO(channel, ...) \
  ::liger::Log::Instance().Add(::liger::LogLevel::Info, LIGER_FILE_NAME ":" LIGER_LINE_TO_STR(__LINE__), channel, __VA_ARGS__);
#define LIGER_LOG_TRACE(channel, ...) \
  ::liger::Log::Instance().Add(::liger::LogLevel::Trace, LIGER_FILE_NAME ":" LIGER_LINE_TO_STR(__LINE__), channel, __VA_ARGS__);
#define LIGER_LOG_WARN(channel, ...) \
  ::liger::Log::Instance().Add(::liger::LogLevel::Warning, LIGER_FILE_NAME ":" LIGER_LINE_TO_STR(__LINE__), channel, __VA_ARGS__);
#define LIGER_LOG_ERROR(channel, ...) \
  ::liger::Log::Instance().Add(::liger::LogLevel::Error, LIGER_FILE_NAME ":" LIGER_LINE_TO_STR(__LINE__), channel, __VA_ARGS__);
#define LIGER_LOG_FATAL(channel, ...) \
  ::liger::Log::Instance().Add(::liger::LogLevel::Fatal, LIGER_FILE_NAME ":" LIGER_LINE_TO_STR(__LINE__), channel, __VA_ARGS__);

#define LIGER_ASSERT(condition, channel, ...) \
  if (!(condition)) {                         \
    LIGER_LOG_FATAL(channel, __VA_ARGS__);    \
    std::terminate();                         \
  }
