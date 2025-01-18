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

namespace openvtt::renderer {
/**
 * @brief A simple camera struct
 *
 * This camera always assumes that up is along (0, 1, 0), i.e. the positive Y axis.
 * It supports non-trivial rotation (around the up axis through a projected point on the XZ plane), forward translation
 * (along the forward vector, projected on the XZ plane), and zooming (translation along the actual forward vector).
 */
struct camera {
  glm::vec3 position{0,3,5}; //!< The position of the camera.
  glm::vec3 forward = normalize(glm::vec3{0,0,0} - position); //!< The forward vector of the camera.
  // glm::vec3 up{0,1,0}; // up is always positive Y
  float speed = 1.0f; //!< The movement speed of the camera.
  float rot_speed = 3.5f; //!< The rotation speed of the camera.
  float zoom_speed = 2.0f; //!< The zoom speed of the camera.

  /**
   * @brief Handles the input for the camera.
   *
   * This function processes user input to control the camera's movement and rotation. To ensure compatibility with Dear
   * IMGUI, this function checks whether any IMGUI window is active, and if so, is a no-op.
   */
  void handle_input();

  /**
   * @brief Renders the camera controls.
   *
   * This function is responsible for rendering the camera's control interface. The interface displays the camera's
   * current position, forward vector, and rotation point (see handle_input()'s implementation), as well as some sliders
   * to control the camera's speeds (movement, rotation, and zoom).
   */
  void render_controls();

  /**
   * @brief Returns the view matrix of the camera.
   */
  [[nodiscard]] inline glm::mat4 view_matrix() const {
    return lookAt(position, position + forward, {0,1,0});
  }

  /**
   * @brief Returns the projection matrix of the camera.
   */
  [[nodiscard]] inline static glm::mat4 projection_matrix() {
    return glm::perspective(glm::radians(45.0f), window::get().aspect_ratio(), 0.1f, 100.0f);
  }

  /**
   * @brief Sets the view and projection matrices in the shader.
   *
   * This function sets the view and projection matrices in the provided shader. It uses the view_matrix() and
   * projection_matrix() functions to get the matrices.
   *
   * @param s The shader to set the matrices in.
   * @param view_loc The location of the view matrix uniform in the shader.
   * @param proj_loc The location of the projection matrix uniform in the shader.
   */
  inline void set_matrices(const shader &s, const unsigned int view_loc, const unsigned int proj_loc) const {
    s.set_mat4(view_loc, view_matrix());
    s.set_mat4(proj_loc, projection_matrix());
  }
};
}

#endif //CAMERA_HPP
