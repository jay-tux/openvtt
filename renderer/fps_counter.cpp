//
// Created by jay on 11/29/24.
//

#include "fps_counter.hpp"
#include "window.hpp"

using namespace gltt::renderer;

void fps_counter::render() {
  const auto &io = window::get().io_data();
  ImGui::Begin("FPS");
  ImGui::Text("%.2f FPS ~ %.2f ms/frame", io.Framerate, 1000.0f / io.Framerate);
  ImGui::End();
}
