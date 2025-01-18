//
// Created by jay on 11/29/24.
//

#ifndef FPS_COUNTER_HPP
#define FPS_COUNTER_HPP

#include <glm/glm.hpp>

namespace openvtt::renderer {

/**
 * @brief A struct for rendering the FPS counter.
 */
struct fps_counter {
  /**
   * @brief Renders the FPS counter.
   * @param mouse_y0 The XZ-position of the mouse, projected onto the ground (y=0).
   */
  static void render(const glm::vec2 &mouse_y0);
};

}

#endif //FPS_COUNTER_HPP