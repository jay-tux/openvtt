#include <iostream>
#include <glm/gtc/random.hpp>

#include "renderer/window.hpp"
#include "renderer/fps_counter.hpp"
#include "renderer/log_view.hpp"
#include "renderer/renderable.hpp"
#include "renderer/fbo.hpp"
#include "renderer/hover_highlighter.hpp"

#include "map/map_parser.hpp"

using namespace openvtt::map;
using namespace openvtt::renderer;

constexpr size_t point_light_count = 10;

int main(int argc, const char **argv) {
  using cache = render_cache;
  auto &win = window::get();
  const auto [
    scene,
    scene_instances,
    requires_highlight,
    requires_instanced_highlight,
    highlight_binding
  ] = map_desc::parse_from("examples/suzannes");

  auto cam = camera{};

  // force window initialization etc
  win.frame_pre();
  win.frame_post();

  phong_lighting lights(0.1f, {
    { true, { .pos = {0, 1, 0}, .diffuse = {0.9, 0.9, 0.6} } }
  });

  std::vector<render_ref> set_base;
  std::vector<instanced_render_ref> set_inst_base;
  std::vector<std::pair<render_ref, single_highlight>> set_highlight;
  std::vector<std::pair<instanced_render_ref, instanced_highlight>> set_inst_highlight;

  for (const auto &r: scene) {
    if (const auto it = requires_highlight.find(r->sh); it != requires_highlight.end())
      set_highlight.emplace_back(r, it->second);
    else
      set_base.emplace_back(r);
  }

  for (const auto &i: scene_instances) {
    if (const auto it = requires_instanced_highlight.find(i->sh); it != requires_instanced_highlight.end())
      set_inst_highlight.emplace_back(i, it->second);
    else
      set_inst_base.emplace_back(i);
  }

  // requires static - otherwise lambda captures get invalidated for some reason
  static single_highlight curr_single{0,0};
  static instanced_highlight curr_instanced{0,0,0};

  const auto lighting_default = setup_phong_shading<point_light_count>(cam, lights);
  const auto lighting_instanced = setup_phong_shading<point_light_count, instanced_renderable>(cam, lights);
  const auto lighting_highlight = setup_phong_shading<point_light_count>(cam, lights, [](const shader_ref &s, const renderable &r) {
    if (r.coll.has_value()) s->set_bool(curr_single.uniform_highlight, (*r.coll)->is_hovered);
  });
  const auto lighting_instanced_highlight = setup_phong_shading<point_light_count, instanced_renderable>(cam, lights, [](const shader_ref &s, const instanced_renderable &r) {
    if (r.coll.has_value()) {
      s->set_bool(curr_instanced.uniform_highlight, (*r.coll)->is_hovered);
      s->set_uint(curr_instanced.uniform_instance_id, (*r.coll)->highlighted_instance);
    }
  });

  using highlighter = hover_highlighter;

  while (!win.should_close()) {
    if (!win.frame_pre()) continue;

    highlighter::reset();

    cam.handle_input();

    highlighter::highlight_checking(cam);

    if (highlight_binding.has_value()) {
      highlighter::bind_highlight_tex(*highlight_binding);

      for (const auto &[s, idx] : requires_highlight) {
        s->set_int(idx.uniform_tex, *highlight_binding);
      }
      for (const auto &[s, idx] : requires_instanced_highlight) {
        s->set_int(idx.uniform_tex, *highlight_binding);
      }
    }

    // perlin_square->draw(cam, [&scales](const shader_ref &sr, const renderable &) {
    //   sr->set_float(4, scales[0]);
    //   sr->set_float(5, scales[1]);
    //   sr->set_float(6, scales[2]);
    //   sr->set_float(7, 0 * static_cast<float>(ImGui::GetTime()));
    // });

    // highlighter::bind_highlight_tex(15);
    // phong->set_int(phong->loc_for("highlight_map"), 15);
    // suzanne->draw(cam, lighting_suzanne);

    // instanced_phong->set_int(instanced_phong->loc_for("highlight_map"), 15);
    // many_monkeys->draw(cam, lighting_monkeys);

    // for (const auto &r : scene) {
    //   r->draw(cam, lighting_suzanne);
    // }
    // for (const auto &i : scene_instances) {
    //   i->draw(cam, lighting_monkeys);
    // }

    for (const auto &r : set_base) r->draw(cam, lighting_default);
    for (const auto &[r, l] : set_highlight) {
      curr_single = l;
      r->draw(cam, lighting_highlight);
    }
    for (const auto &r : set_inst_base) r->draw(cam, lighting_instanced);
    for (const auto &[r, l] : set_inst_highlight) {
      curr_instanced = l;
      r->draw(cam, lighting_instanced_highlight);
    }

    cache::draw_colliders(cam);

    fps_counter::render();
    log_view::render();
    cam.render_controls();
    cache::detail_window();
    lights.detail_window();
    highlighter::get_fbo().draw_texture_imgui("Highlight Buffer", 256, 256);
    // ImGui::ShowMetricsWindow();

    // ImGui::Begin("Perlin Parameters");
    // ImGui::SliderFloat("scale r", scales, -5.0f, 50.0f);
    // ImGui::SliderFloat("scale g", scales + 1, -5.0f, 50.0f);
    // ImGui::SliderFloat("scale b", scales + 2, -5.0f, 50.0f);
    // ImGui::End();

    win.frame_post();
  }

  return 0;
}