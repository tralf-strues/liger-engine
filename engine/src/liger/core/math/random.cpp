/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file random.cpp
 * @date 2023-09-12
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

#include "liger/core/math/random.hpp"

#include <glm/gtc/random.hpp>

using namespace liger;

Random Random::instance_;

Random& Random::Instance() { return instance_; }

Random::Random() : generator_(device_()) {}

float Random::InRange(float start, float end) {
  std::uniform_real_distribution<float> dist(start, end);
  return dist(generator_);
}

glm::vec3 Random::InRange(const glm::vec3& start, const glm::vec3& end) {
  return glm::vec3(InRange(start.x, end.x), InRange(start.y, end.y), InRange(start.z, end.z));
}

glm::vec3 Random::InUnitSphere(const glm::vec3& center) {
  return center + glm::sphericalRand(1.0f);
}