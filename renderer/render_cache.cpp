//
// Created by jay on 11/30/24.
//

#include "gl_macros.hpp"
#include <imgui.h>

#include "render_cache.hpp"

using namespace openvtt::renderer;

void render_cache::detail_window() {
  ImGui::Begin("Render Cache Contents");

  ImGui::Text("%d objects\n%d shaders\n%d textures", objects.size(), shaders.size(), textures.size());
  ImGui::SameLine();
  ImGui::Checkbox("Render Colliders", &render_colliders);


  int i = 0;
  if (ImGui::BeginChild("Renderables")) {
    if (ImGui::CollapsingHeader("Single objects")) {
      ImGui::Indent(16.0f);
      for (auto &r: renderables) {
        ImGui::PushID(i);
        if (ImGui::CollapsingHeader(r.name.empty() ? "(nameless object)" : r.name.c_str())) {
          ImGui::Indent(16.0f);
          ImGui::Checkbox("Active", &r.active);
          ImGui::InputFloat3("Position", &r.position.x);
          ImGui::InputFloat3("Rotation", &r.rotation.x);
          ImGui::InputFloat3("Scale", &r.scale.x);
          ImGui::Unindent(16.0f);
        }
        ImGui::PopID();
        i++;
      }
      ImGui::Unindent(16.0f);
    }

    if (ImGui::CollapsingHeader("Instanced objects")) {
      ImGui::Indent(16.0f);
      for (auto &r: instanced_renderables) {
        ImGui::PushID(i);
        if (ImGui::CollapsingHeader(r.name.empty() ? "(nameless object)" : r.name.c_str())) {
          ImGui::Indent(16.0f);
          ImGui::Checkbox("Active", &r.active);
          std::string txt = std::format("{} instances", r.obj->instance_count());
          ImGui::Text(txt.c_str());
          if (r.coll.has_value()) {
            txt = std::format("{} instances(collider)", (*r.coll)->instance_count());
            ImGui::Text(txt.c_str());
          }
          ImGui::Unindent(16.0f);
        }
        ImGui::PopID();
        i++;
      }
      ImGui::Unindent(16.0f);
    }
  }
  ImGui::EndChild();
  ImGui::End();
}

render_ref render_cache::duplicate(const render_ref &ref, const std::string &name, const std::optional<glm::vec3> &pos,
                                   const std::optional<glm::vec3> &rot, const std::optional<glm::vec3> &scale) {
  const auto &r = *ref;
  auto &out = renderables.emplace_back(
    name, r.obj, r.sh,
    uniforms{r.model_loc, r.view_loc, r.proj_loc, r.model_inv_t_loc},
    r.textures
  );

  if (pos.has_value()) out.position = *pos;
  if (rot.has_value()) out.rotation = *rot;
  if (scale.has_value()) out.scale = *scale;

  if (r.coll.has_value()) {
    colliders.emplace_back(**r.coll);
    out.coll = collider_ref{colliders.size() - 1};
  }

  return render_ref{renderables.size() - 1};
}

void render_cache::draw_colliders(const camera &cam) {
  static unsigned int model_loc, view_loc, proj_loc, highlighted_loc;
  static unsigned int view_loc_inst, proj_loc_inst, highlighted_loc_inst, highlight_idx_loc;

  if (!render_colliders) return;

  if (!collider_shader.has_value()) {
    collider_shader = load<shader>("basic_mvp", "collider");
    model_loc = (*collider_shader)->loc_for("model");
    view_loc = (*collider_shader)->loc_for("view");
    proj_loc = (*collider_shader)->loc_for("projection");
    highlighted_loc = (*collider_shader)->loc_for("highlighted");
  }
  if (!collider_instanced_shader.has_value()) {
    collider_instanced_shader = load<shader>("basic_mvp_instanced", "collider_instanced");
    view_loc_inst = (*collider_instanced_shader)->loc_for("view");
    proj_loc_inst = (*collider_instanced_shader)->loc_for("projection");
    highlighted_loc_inst = (*collider_instanced_shader)->loc_for("highlighted");
    highlight_idx_loc = (*collider_instanced_shader)->loc_for("instance_id");
  }

  const auto &sh = **collider_shader;
  const auto &i_sh = **collider_instanced_shader;
  sh.activate();
  cam.set_matrices(sh, view_loc, proj_loc);

  for (const auto &r : renderables) {
    if (r.active && r.coll.has_value()) {
      sh.set_mat4(model_loc, r.model());
      sh.set_bool(highlighted_loc, (*r.coll)->is_hovered);
      (*r.coll)->draw();
    }
  }

  i_sh.activate();
  cam.set_matrices(i_sh, view_loc_inst, proj_loc_inst);

  for (const auto &r : instanced_renderables) {
    if (r.active && r.coll.has_value()) {
      i_sh.set_bool(highlighted_loc_inst, (*r.coll)->is_hovered);
      i_sh.set_uint(highlight_idx_loc, (*r.coll)->highlighted_instance);
      (*r.coll)->draw_all();
    }
  }
}

glm::vec2 render_cache::mouse_y0(const camera &cam) {
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

  const float alpha = -near_world.y / (far_world.y - near_world.y);
  const auto calc = near_world + alpha * (far_world - near_world);
  return {calc.x, calc.z};
}


render_cache::collision_res render_cache::mouse_over(const camera &cam) {
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
    if (renderables[i].active && renderables[i].coll.has_value()) {
      const float d = (*renderables[i].coll)->ray_intersect(r, renderables[i].model());

      if (d < t_dist) {
        t_dist = d;
        res = render_ref{i};
      }
    }
  }

  // --- Check for collisions with instanced colliders ---
  std::optional<instanced_render_ref> inst_res = std::nullopt;
  size_t inst_idx = 0;
  for (size_t i = 0; i < instanced_renderables.size(); i++) {
    if (instanced_renderables[i].active && instanced_renderables[i].coll.has_value()) {
      // ReSharper disable once CppTooWideScopeInitStatement
      const auto [d, idx] = (*instanced_renderables[i].coll)->ray_intersect_any(r);

      if (d < t_dist) {
        t_dist = d;
        inst_res = instanced_render_ref{i};
        inst_idx = idx;
      }
    }
  }

  return inst_res.has_value() ? collision_res{std::pair{*inst_res, inst_idx}} :
          res.has_value() ? collision_res{*res} : collision_res{no_collision{}};
}