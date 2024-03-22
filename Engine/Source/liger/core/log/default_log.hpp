/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file default_log.hpp
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

#pragma once

#include <liger/core/log/log.hpp>

#define LIGER_LINE_TO_STR(x) LIGER_TO_STR(x)
#define LIGER_TO_STR(x) #x

#ifdef __FILE_NAME__
#define LIGER_FILE_NAME __FILE_NAME__
#else
#define LIGER_FILE_NAME __FILE__
#endif

#define LIGER_LOG_INFO(channel, ...) \
  ::liger::default_log::g_log.Message(::liger::LogLevel::kInfo, LIGER_FILE_NAME ":" LIGER_LINE_TO_STR(__LINE__), channel, __VA_ARGS__);
#define LIGER_LOG_TRACE(channel, ...) \
  ::liger::default_log::g_log.Message(::liger::LogLevel::kTrace, LIGER_FILE_NAME ":" LIGER_LINE_TO_STR(__LINE__), channel, __VA_ARGS__);
#define LIGER_LOG_WARN(channel, ...) \
  ::liger::default_log::g_log.Message(::liger::LogLevel::kWarning, LIGER_FILE_NAME ":" LIGER_LINE_TO_STR(__LINE__), channel, __VA_ARGS__);
#define LIGER_LOG_ERROR(channel, ...) \
  ::liger::default_log::g_log.Message(::liger::LogLevel::kError, LIGER_FILE_NAME ":" LIGER_LINE_TO_STR(__LINE__), channel, __VA_ARGS__);
#define LIGER_LOG_FATAL(channel, ...) \
  ::liger::default_log::g_log.Message(::liger::LogLevel::kFatal, LIGER_FILE_NAME ":" LIGER_LINE_TO_STR(__LINE__), channel, __VA_ARGS__);

#define LIGER_ASSERT(condition, channel, ...) \
  if (!(condition)) {                         \
    LIGER_LOG_FATAL(channel, __VA_ARGS__);    \
    std::terminate();                         \
  }

namespace liger::default_log {

extern Log g_log;

}  // namespace liger::default_log