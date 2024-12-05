//
// Created by jay on 12/5/24.
//

#ifndef GLM_WRAPPER_HPP
#define GLM_WRAPPER_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace openvtt::renderer {
template <typename F>
concept matrix_transformer = requires(F &&f, const glm::mat4 &m)
{
  { f(m) } -> std::convertible_to<glm::mat4>;
};

template <matrix_transformer T>
constexpr glm::mat4 operator|(const glm::mat4 &in, T &&f) {
  return f(in);
}

constexpr matrix_transformer auto translation(const glm::vec3 &delta) {
  return [&delta](const glm::mat4 &m) { return translate(m, delta); };
}
constexpr matrix_transformer auto rescale(const glm::vec3 &scale) {
  return [&scale](const glm::mat4 &m) { return glm::scale(m, scale); };
}
constexpr matrix_transformer auto yaw(float angle) {
  return [angle](const glm::mat4 &m) { return rotate(m, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f)); };
}
constexpr matrix_transformer auto pitch(float angle) {
  return [angle](const glm::mat4 &m) { return rotate(m, glm::radians(angle), glm::vec3(1.0f, 0.0f, 0.0f)); };
}
constexpr matrix_transformer auto roll(float angle) {
  return [angle](const glm::mat4 &m) { return rotate(m, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)); };
}
}

#endif //GLM_WRAPPER_HPP
