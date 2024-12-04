//
// Created by jay on 11/30/24.
//

#include <imgui.h>

#include "render_cache.hpp"

using namespace openvtt::renderer;

void render_cache::detail_window() {
  ImGui::Begin("Render Cache Contents");

  ImGui::Text("%d objects\n%d shaders\n%d textures", objects.size(), shaders.size(), textures.size());
  ImGui::SameLine();
  ImGui::Checkbox("Render Colliders", &render_colliders);


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

void render_cache::draw_colliders(const camera &cam) {
  static unsigned int model_loc, view_loc, proj_loc, highlighted_loc;

  if (!render_colliders) return;

  if (!collider_shader.has_value()) {
    collider_shader = load_shader("basic_mvp", "collider");
    model_loc = (*collider_shader)->loc_for("model");
    view_loc = (*collider_shader)->loc_for("view");
    proj_loc = (*collider_shader)->loc_for("projection");
    highlighted_loc = (*collider_shader)->loc_for("highlighted");
  }

  const auto &sh = **collider_shader;
  sh.activate();
  cam.set_matrices(sh, view_loc, proj_loc);

  for (const auto &r : renderables) {
    if (r.active && r.coll.has_value()) {
      sh.set_mat4(model_loc, r.model());
      sh.set_bool(highlighted_loc, (*r.coll)->is_hovered);
      (*r.coll)->draw();
    }
  }
}

std::optional<render_ref> render_cache::mouse_over(const camera &cam) {
  // --- Construct camera to mouse ray ---
  const auto &io = window::get().io_data();
  const auto mouse = io.MousePos;
  const auto w_h = io.DisplaySize;
  const glm::vec2 ndc = { 2 * mouse.x / w_h.x - 1, 1 - 2 * mouse.y / w_h.y };
  const glm::vec4 clip_near = { ndc.x, ndc.y, -1, 1 };
  const glm::vec4 clip_far = { ndc.x, ndc.y, 1, 1 };
  const glm::mat4 clip_to_world = glm::inverse(camera::projection_matrix() * cam.view_matrix());
  const auto near = clip_to_world * clip_near;
  const auto far = clip_to_world * clip_far;

  const glm::vec3 near_world{near.x / near.w, near.y / near.w, near.z / near.w};
  const glm::vec3 far_world{far.x / far.w, far.y / far.w, far.z / far.w};

  const glm::vec3 dir = normalize(far_world - near_world);
  const ray r{cam.position, dir};

  // --- Check for collisions ---
  std::optional<render_ref> res = std::nullopt;
  float t_dist = INFINITY;
  for (size_t i = 0; i < renderables.size(); i++) {
    if (renderables[i].coll.has_value()) {
      const float d = (*renderables[i].coll)->ray_intersect(r, renderables[i].model());
      if (d < t_dist) {
        t_dist = d;
        res = render_ref{i};
      }
    }
  }
  return res;
}