/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file main.cpp
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

#include "liger/core/core.hpp"

int main() {
  liger::Log log;
  log.AddWriter(std::make_unique<liger::ConsoleLogWriter>(liger::kDefaultConsoleLogStyle));

  liger::int32 i = 0;
  log.Message(liger::LogLevel::kInfo,    liger::LigerLogChannel::kNone,    "Info none {0}", i++);
  log.Message(liger::LogLevel::kTrace,   liger::LigerLogChannel::kCore,    "Trace core {0}", i++);
  log.Message(liger::LogLevel::kWarning, liger::LigerLogChannel::kRender,  "Warning render {0}", i++);
  log.Message(liger::LogLevel::kError,   liger::LigerLogChannel::kECS,     "Error ecs {0}", i++);
  log.Message(liger::LogLevel::kFatal,   liger::LigerLogChannel::kPhysics, "Fatal physics {0}", i++);

  // std::cout << "Liger Sandbox" << std::endl;

  return 0;
}
