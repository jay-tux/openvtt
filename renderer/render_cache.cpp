//
// Created by jay on 11/30/24.
//

#include <imgui.h>

#include "render_cache.hpp"

using namespace openvtt::renderer;

void render_cache::detail_window() {
  ImGui::Begin("Render Cache Contents");
  ImGui::Text("%d objects\n%d shaders\n%d textures", objects.size(), shaders.size(), textures.size());
  if (ImGui::BeginChild("Renderables")) {
    int i = 0;
    for (auto &r: renderables) {
      ImGui::PushID(i);
      if (ImGui::CollapsingHeader(r.name.empty() ? "(nameless object)" : r.name.c_str())) {
        ImGui::Checkbox("Active", &r.active);
        ImGui::InputFloat3("Position", &r.position.x);
        ImGui::InputFloat3("Rotation", &r.rotation.x);
        ImGui::InputFloat3("Scale", &r.scale.x);
      }
      ImGui::PopID();
      i++;
    }
  }
  ImGui::EndChild();
  ImGui::End();
}
