//
// Created by jay on 12/5/24.
//

#ifndef GLM_WRAPPER_HPP
#define GLM_WRAPPER_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace openvtt::renderer {
/**
 * @brief A concept that checks if a type is a matrix transformer.
 *
 * To be a matrix transformer, the type must be callable with a glm::mat4 and return a glm::mat4 (have the signature
 * `(const glm::mat4 &) -> glm::mat4`).
 */
template <typename F>
concept matrix_transformer = requires(F &&f, const glm::mat4 &m)
{
  { f(m) } -> std::convertible_to<glm::mat4>;
};

/**
 * @brief A shorthand for applying a matrix transformer to a glm::mat4.
 * @param in The matrix to transform.
 * @param f The transformer to apply.
 *
 * This is a shorthand for `f(in)`, allowing for functional-style pipelines, where the matrix is transformed by a series
 * of operations in a more readable manner. Note that OpenGL/GLM requires the transformations to be applied in the
 * reverse order they are written, which isn't changed by this operator.
 */
template <matrix_transformer T>
constexpr glm::mat4 operator|(const glm::mat4 &in, T &&f) {
  return f(in);
}

/**
 * @brief Constructs a matrix transformer that applies a translation to a matrix.
 * @param delta The translation to apply.
 * @return A matrix transformer that applies the translation.
 *
 * This is a convenience wrapper function around `glm::translate(in, delta)`.
 */
constexpr matrix_transformer auto translation(const glm::vec3 &delta) {
  return [&delta](const glm::mat4 &m) { return translate(m, delta); };
}
/**
 * @brief Constructs a matrix transformer that applies a scale to a matrix.
 * @param scale The angle to rotate by.
 * @return A matrix transformer that applies the scale.
 *
 * This is a convenience wrapper function around `glm::scale(in, scale)`.
 */
constexpr matrix_transformer auto rescale(const glm::vec3 &scale) {
  return [&scale](const glm::mat4 &m) { return glm::scale(m, scale); };
}
/**
* @brief Constructs a matrix transformer that applies a yaw-rotation to a matrix (around the Y-axis).
* @param angle The angle to rotate by (in degrees).
* @return A matrix transformer that applies the yaw-rotation.
*
* This is a convenience wrapper function around `glm::rotate(in, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f))`.
*/
constexpr matrix_transformer auto yaw(float angle) {
  return [angle](const glm::mat4 &m) { return rotate(m, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f)); };
}
/**
 * @brief Constructs a matrix transformer that applies a pitch-rotation to a matrix (around the X-axis).
 * @param angle The angle to rotate by (in degrees).
 * @return A matrix transformer that applies the pitch-rotation.
 *
 * This is a convenience wrapper function around `glm::rotate(in, glm::radians(angle), glm::vec3(1.0f, 0.0f, 0.0f))`.
 */
constexpr matrix_transformer auto pitch(float angle) {
  return [angle](const glm::mat4 &m) { return rotate(m, glm::radians(angle), glm::vec3(1.0f, 0.0f, 0.0f)); };
}
/**
 * @brief Constructs a matrix transformer that applies a roll-rotation to a matrix (around the Z-axis).
 * @param angle The angle to rotate by (in degrees).
 * @return A matrix transformer that applies the roll-rotation.
 *
 * This is a convenience wrapper function around `glm::rotate(in, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f))`.
 */
constexpr matrix_transformer auto roll(float angle) {
  return [angle](const glm::mat4 &m) { return rotate(m, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)); };
}
}

#endif //GLM_WRAPPER_HPP
