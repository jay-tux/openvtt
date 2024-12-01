//
// Created by jay on 11/30/24.
//

#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "window.hpp"
#include "shader.hpp"

namespace gltt::renderer {
struct camera {
  glm::vec3 position{0,3,-5};
  glm::vec3 forward = normalize(glm::vec3{0,0,0} - position);
  // glm::vec3 up{0,1,0}; // up is always positive Y
  float speed = 1.0f;
  float rot_speed = 1.5f;
  float zoom_speed = 2.0f;

  void handle_input();

  void render_controls();

  [[nodiscard]] inline glm::mat4 view_matrix() const {
    return lookAt(position, position + forward, {0,1,0});
  }
  [[nodiscard]] inline static glm::mat4 projection_matrix() {
    return glm::perspective(glm::radians(45.0f), window::get().aspect_ratio(), 0.1f, 100.0f);
  }

  inline void set_matrices(const shader &s, const unsigned int view_loc, const unsigned int proj_loc) const {
    s.set_mat4(view_loc, view_matrix());
    s.set_mat4(proj_loc, projection_matrix());
  }
};
}

#endif //CAMERA_HPP
