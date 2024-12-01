//
// Created by jay on 11/30/24.
//

#include "renderable.hpp"

using namespace openvtt::renderer;

namespace {
void color_picker_dialog(const char *label, glm::vec3 &color) {
  const auto name = std::format("picker##{}", label);
  if (ImGui::ColorButton(name.c_str(), ImVec4{color.x, color.y, color.z, 1.0f})) {
    ImGui::OpenPopup(name.c_str());
  }
  ImGui::SameLine();
  ImGui::Text(label);

  if (ImGui::BeginPopup(name.c_str())) {
    ImGui::ColorPicker3(label, &color.x, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_PickerHueWheel);
    ImGui::EndPopup();
  }
}
}

void phong_lighting::detail_window() {
  ImGui::Begin("Phong Lighting Parameters");
  ImGui::SliderFloat("Ambient Strength", &ambient_strength, 1e-5f, 1.0f);
  for (size_t i = 0; i < points.size(); ++i) {
    ImGui::PushID(i);
    if (ImGui::CollapsingHeader(std::format("Point Light #{}", i + 1).c_str())) {
      ImGui::Checkbox("Enabled", &points[i].first);
      ImGui::InputFloat3("Position", &points[i].second.pos.x);
      color_picker_dialog("Diffuse", points[i].second.diffuse);
      color_picker_dialog("Specular", points[i].second.specular);
      ImGui::InputFloat3("Attenuation (1, t, t²)", &points[i].second.attenuation.x);
    }
    ImGui::PopID();
  }
  ImGui::Spacing();
  window::get().with_nerd_icons([this] {
    if (ImGui::Button(reinterpret_cast<const char *>(u8"")))
      add_point(false, { .pos = {0,0,0}, .diffuse = {1,1,1} });
  });
  ImGui::End();
}