#include <iostream>
#include <glm/gtc/random.hpp>

#include "renderer/window.hpp"
#include "renderer/fps_counter.hpp"
#include "renderer/log_view.hpp"
#include "renderer/renderable.hpp"
#include "renderer/fbo.hpp"
#include "renderer/hover_highlighter.hpp"

#include "map/map_parser.hpp"
#include "renderer/gizmos.hpp"

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
    highlight_binding,
    enable_axes
  ] = map_desc::parse_from("examples/suzannes");

  auto cam = camera{};

  // force window initialization etc
  win.frame_pre();
  win.frame_post();

  phong_lighting lights(0.1f, {
      .direction = {1, -1, 0},
      .diffuse = {0.75, 0.75, 0.75},
      .specular = {1, 1, 1}
    },
    {
      { true, { .pos = {0, 1, 0}, .diffuse = {0.9, 0.9, 0.6} } }
  });
  bool draw_lights = false;

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

  axes ax{};

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

    if (enable_axes) ax.draw(cam);

    const auto mouse = cache::mouse_y0(cam);
    // ax.draw(cam, {mouse.x, 0, mouse.y}, 0.25f);

    if (draw_lights) lights.draw_actives(ax, cam);

    fps_counter::render(mouse);
    log_view::render();
    cam.render_controls();
    cache::detail_window();
    lights.detail_window(&draw_lights);
    highlighter::get_fbo().draw_texture_imgui("Highlight Buffer", 256, 256);

    win.frame_post();
  }

  return 0;
}