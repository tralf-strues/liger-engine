/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file time.cpp
 * @date 2023-09-11
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

#include <liger/core/log/default_log.hpp>
#include <liger/core/time.hpp>

namespace liger {

Timer::Timer() { Reset(); }

void Timer::Reset() { start_ = std::chrono::high_resolution_clock::now(); }

float Timer::Elapsed() {
  using namespace std::chrono;
  return duration_cast<nanoseconds>(high_resolution_clock::now() - start_).count() * 1e-9f;
}

float Timer::ElapsedMs() { return Elapsed() * 1e3f; }

ScopedTimer::ScopedTimer(const std::string_view channel, const std::string_view message)
    : channel_(channel), message_(message) {}

ScopedTimer::~ScopedTimer() {
  LIGER_LOG_TRACE(channel_, "{} - {:.{}f}ms", message_, timer_.ElapsedMs(), 3);
}

}  // namespace liger