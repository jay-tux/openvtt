//
// Created by jay on 1/18/25.
//

#ifndef GIZMOS_HPP
#define GIZMOS_HPP

#include "camera.hpp"
#include "object.hpp"
#include "shader.hpp"
#include "glm_wrapper.hpp"

namespace openvtt::renderer {
class axes {
public:
  axes() : obj{render_object::load_from("axis")}, s{shader::load_from("axes", "axes")} {}

  void draw(const camera &cam, const glm::vec3 &origin = {0, 0, 0}, float length = 1.0f) const;

private:
  render_object obj;
  shader s;
  static inline auto x_rot = glm::mat4{1.0f};
  static inline auto y_rot = x_rot | roll(90.0f);
  static inline auto z_rot = x_rot | yaw(90.0f);
  static constexpr glm::vec3 x_color = {1, 0, 0};
  static constexpr glm::vec3 y_color = {0, 1, 0};
  static constexpr glm::vec3 z_color = {0, 0, 1};
};
}

#endif //GIZMOS_HPP
