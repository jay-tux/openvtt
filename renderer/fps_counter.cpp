//
// Created by jay on 11/29/24.
//

#include "fps_counter.hpp"
#include "window.hpp"

using namespace openvtt::renderer;

void fps_counter::render(const glm::vec2 &mouse_y0) {
  const auto &io = window::get().io_data();
  ImGui::Begin("FPS");
  ImGui::Text(
    "%.2f FPS ~ %.2f ms/frame",
    io.Framerate, 1000.0f / io.Framerate
  );
  ImGui::Text(
    "Mouse hovering over (%.2f, %.2f)",
    mouse_y0.x, mouse_y0.y
  );
  ImGui::End();
}
