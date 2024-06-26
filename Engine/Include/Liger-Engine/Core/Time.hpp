/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Time.hpp
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

#include <chrono>
#include <string>

namespace liger {

/**
 * @brief Utility class for measuring time.
 */
class Timer {
 public:
  /**
   * @brief Upon construction, timer starts at time = 0.
   */
  Timer();

  /**
   * @brief Resets the timer to time = 0.
   */
  void Reset();

  /**
   * @brief Elapsed time in seconds since either construction of the timer or last call to @ref Reset().
   * @return Time in seconds.
   */
  float Elapsed() const;

  /**
   * @brief Elapsed time in milliseconds since either construction of the timer or last call to @ref Reset().
   * @return Time in milliseconds.
   */
  float ElapsedMs() const;

 private:
  std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

/**
 * @brief Utility class for measuring time taking to execute a scope of code.
 */
class ScopedTimer {
 public:
  /**
   * @param channel Log channel for the timer.
   * @param message Message to log upon destruction.
   */
  explicit ScopedTimer(std::string_view channel, std::string_view message);

  /**
   * @brief Logs the lifetime of the object with specified log channel and name.
   */
  ~ScopedTimer();

 private:
  std::string channel_;
  std::string message_;
  Timer       timer_;
};

/**
* @brief Utility class for measuring frame time.
*/
class FrameTimer {
 public:
  /**
   * @brief Time point at which current frame started.
   * @return Time point in seconds.
   */
  float AbsoluteTime() const;

  /**
   * @brief Same as @ref AbsoluteTime(), but in milliseconds.
   * @return Time point in milliseconds.
   */
  float AbsoluteTimeMs() const;

  /**
   * @brief Time it took to process the previous frame.
   * @return Delta time in seconds.
   */
  float DeltaTime() const;

  /**
   * @brief Same as @ref DeltaTime(), but in milliseconds.
   * @return Delta time in milliseconds.
   */
  float DeltaTimeMs() const;

  /**
   * @brief Get internal timer.
   * @return Internal timer reference.
   */
  const Timer& GetTimer() const;

  /**
   * @brief Calculate Frames-per-second based on the current delta time.
   * @return FPS.
   */
  float FPS() const;

  /**
   * @brief Frame number, i.e. how many times @ref BeginFrame() was called minus 1 (indexing starts from 0).
   * @return Frame number.
   */
  uint64_t FrameNumber() const;

  /**
   * @return Whether current frame is the first one (i.e. FrameNumber() == 0).
   */
  bool FirstFrame() const;

  /**
   * @brief Proceed to the next frame.
   */
  void BeginFrame();

 private:
  static constexpr uint64_t kUndefinedFrameNumber = std::numeric_limits<uint64_t>::max();

  Timer    timer_;
  uint64_t frame_number_{kUndefinedFrameNumber};
  float    absolute_time_{0.0f};
  float    delta_time_{0.0f};
};

}  // namespace liger