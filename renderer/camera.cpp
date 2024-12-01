//
// Created by jay on 11/30/24.
//

#include "camera.hpp"

using namespace openvtt::renderer;

void camera::handle_input() {
  if (ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) return; // let ImGui handle input

  const window &w = window::get();
  const float dt = w.delta_time_s();

  // ~~~~ Part 1: rotating ~~~~
  // get rotation point
  glm::vec3 rot_point;
  glm::vec3 rot_point_zero;
  if (std::abs(forward.y) >= 1e-6f) {
    const float t = -position.y / forward.y;
    rot_point = rot_point_zero = position + t * forward;
    rot_point.y = position.y;
  }
  else {
    rot_point_zero = {0,0,0};
    rot_point = {0, position.y, 0};
  }

  if (ImGui::IsKeyDown(ImGuiKey_A)) {
    // rotate clockwise around rot_point ~> negative angle
    const glm::mat4 rotation = rotate(glm::mat4(1.0f), glm::radians(-rot_speed * glm::two_pi<float>() * dt), {0,1,0});
    position = glm::vec3(rotation * glm::vec4(position - rot_point, 1.0f)) + rot_point;
    forward = normalize(rot_point_zero - position);
  }
  if (ImGui::IsKeyDown(ImGuiKey_D)) {
    // rotate counterclockwise around rot_point ~> positive angle
    const glm::mat4 rotation = rotate(glm::mat4(1.0f), glm::radians(rot_speed * glm::two_pi<float>() * dt), {0,1,0});
    position = glm::vec3(rotation * glm::vec4(position - rot_point, 1.0f)) + rot_point;
    forward = normalize(rot_point_zero - position);
  }

  // ~~~~ Part 2: translating ~~~~
  // get translation forward vector
  const glm::vec3 right = normalize(cross({0,1,0}, normalize(forward)));
  const glm::vec3 translate_forward = normalize(cross(right, {0,1,0}));

  if (ImGui::IsKeyDown(ImGuiKey_W)) {
    position += speed * dt * translate_forward;
  }
  if (ImGui::IsKeyDown(ImGuiKey_S)) {
    position -= speed * dt * translate_forward;
  }

  // ~~~~ Part 3: zooming ~~~~
  if (ImGui::GetIO().MouseWheel != 0) {
    position += zoom_speed * 25 * dt * (ImGui::GetIO().MouseWheel > 0 ? 1.0f : -1.0f) * forward;
  }
  if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
    position += zoom_speed * dt * -ImGui::GetIO().MouseDelta.y * forward;
  }
}

void camera::render_controls() {
  glm::vec3 rot_point;
  if (std::abs(forward.y) >= 1e-6f) {
    const float t = -position.y / forward.y;
    rot_point = position + t * forward;
    rot_point.y = position.y;
  }
  else {
    rot_point = {0, position.y, 0};
  }

  ImGui::Begin("Camera controls");
  ImGui::Text(
    "Position: (%.2f, %.2f, %.2f)\nForward: (%.2f, %.2f, %.2f)\nRotation pt: (%.2f, %.2f, %.2f)",
    position.x, position.y, position.z,
    forward.x, forward.y, forward.z,
    rot_point.x, rot_point.y, rot_point.z
  );
  ImGui::SliderFloat("Move speed", &speed, 0.001f, 10.0f);
  ImGui::SliderFloat("Rotate speed", &rot_speed, 0.001f, 10.0f);
  ImGui::SliderFloat("Zoom speed", &zoom_speed, 0.001f, 10.0f);
  ImGui::End();
}
