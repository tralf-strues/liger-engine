/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Transform3D.hpp
 * @date 2024-04-30
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

#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

namespace liger {

struct Transform3D {
  constexpr static glm::vec3 kForward {0, 0, -1};
  constexpr static glm::vec3 kUp      {0, 1, 0};
  constexpr static glm::vec3 kRight   {1, 0, 0};

  inline Transform3D() = default;

  /**
   * @brief Reconstruct transform from matrix.
   * @warning Prone to precision errors! Should be avoided when possible.
   * @param matrix
   */
  inline explicit Transform3D(const glm::mat4& matrix);

  inline glm::mat4 TranslationMatrix() const;
  inline glm::mat4 RotationMatrix() const;
  inline glm::mat4 ScaleMatrix() const;

  inline glm::mat4 Matrix() const;
  inline glm::mat4 InverseMatrix() const;

  inline glm::vec3 Forward() const;
  inline glm::vec3 Up() const;
  inline glm::vec3 Right() const;

  inline void Rotate(float angle, const glm::vec3& axis);

  glm::vec3 position{0.0f};
  glm::quat rotation{};
  glm::vec3 scale{1.0f};
};

inline Transform3D::Transform3D(const glm::mat4& matrix) {
  glm::vec3 decomposed_scale;
  glm::quat decomposed_rotation;
  glm::vec3 decomposed_position;
  glm::vec3 decomposed_skew;
  glm::vec4 decomposed_perspective;
  glm::decompose(matrix, decomposed_scale, decomposed_rotation, decomposed_position, decomposed_skew,
                 decomposed_perspective);

  position = decomposed_position;
  rotation = decomposed_rotation;
  scale    = decomposed_scale;
}

inline glm::mat4 Transform3D::TranslationMatrix() const {
  return glm::translate(glm::identity<glm::mat4>(), position);
}

inline glm::mat4 Transform3D::RotationMatrix() const {
  return glm::mat4_cast(rotation);
}

inline glm::mat4 Transform3D::ScaleMatrix() const {
  return glm::scale(glm::identity<glm::mat4>(), scale);
}

inline glm::mat4 Transform3D::Matrix() const {
  return TranslationMatrix() * RotationMatrix() * ScaleMatrix();
}

/**
 * Calculates the inverse of a transformation matrix:
 * M = T * R * S
 * M^-1 = S^-1 * R^-1 * T^-1 = S^-1 * R^T * T^-1
 *
 * Inverse of a scale matrix is just a scale matrix from vec3{1/sx, 1/sy, 1/sz}.
 *
 * Inverse of a rotation matrix is simply its transpose, because rotation
 * matrices are orthogonal.
 *
 * Inverse of a translation matrix is simply a translation matrix composed
 * of negative values of the given translation.
 */
inline glm::mat4 Transform3D::InverseMatrix() const {
  const auto identity = glm::identity<glm::mat4>();
  return glm::scale(identity, 1.0f / scale) * glm::transpose(RotationMatrix()) * glm::translate(identity, -position);
}

inline glm::vec3 Transform3D::Forward() const { return rotation * glm::vec4(kForward, 1.0f); }
inline glm::vec3 Transform3D::Up()      const { return rotation * glm::vec4(kUp,      1.0f); }
inline glm::vec3 Transform3D::Right()   const { return rotation * glm::vec4(kRight,   1.0f); }

inline void Transform3D::Rotate(float angle, const glm::vec3& axis) {
  rotation = glm::angleAxis(angle, axis) * rotation;
}

}  // namespace liger