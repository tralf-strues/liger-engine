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

#include "liger/core/log/log.hpp"

#define LIGER_LOG_INFO(channel, ...)  liger::default_log::g_Log.Message(liger::LogLevel::kInfo,    channel, __VA_ARGS__);
#define LIGER_LOG_TRACE(channel, ...) liger::default_log::g_Log.Message(liger::LogLevel::kTrace,   channel, __VA_ARGS__);
#define LIGER_LOG_WARN(channel, ...)  liger::default_log::g_Log.Message(liger::LogLevel::kWarning, channel, __VA_ARGS__);
#define LIGER_LOG_ERROR(channel, ...) liger::default_log::g_Log.Message(liger::LogLevel::kError,   channel, __VA_ARGS__);
#define LIGER_LOG_FATAL(channel, ...) liger::default_log::g_Log.Message(liger::LogLevel::kFatal,   channel, __VA_ARGS__);

#define LIGER_ASSERT(condition, channel, ...) \
  if (!(condition)) {                         \
    LIGER_LOG_FATAL(channel, __VA_ARGS__);    \
    std::terminate();                         \
  }

namespace liger {
namespace default_log {

extern Log g_Log;

Log CreateDefaultLog();

}  // namespace default_log
}  // namespace liger