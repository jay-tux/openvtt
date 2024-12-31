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

int main(int argc, const char **argv) {
  using cache = render_cache;
  auto &win = window::get();
  auto map = map_desc::parse_from("examples/suzannes");

  // constexpr glm::vec3 positions[5] {
  //   glm::vec3(-0.23f, 0.45f, -0.78f) * 3.0f,
  //   glm::vec3(0.67f, -0.34f, 0.12f) * 3.0f,
  //   glm::vec3(-0.56f, 0.89f, -0.45f) * 3.0f,
  //   glm::vec3(0.12f, -0.67f, 0.34f) * 3.0f,
  //   glm::vec3(-0.78f, 0.23f, -0.56f) * 3.0f
  // };
  // const glm::vec3 rotations[5] {
  //   glm::sphericalRand(1.0f) * 360.0f,
  //   glm::sphericalRand(1.0f) * 360.0f,
  //   glm::sphericalRand(1.0f) * 360.0f,
  //   glm::sphericalRand(1.0f) * 360.0f,
  //   glm::sphericalRand(1.0f) * 360.0f
  // };
  // constexpr glm::vec3 scales_[5] {
  //   glm::vec3{0.18}, glm::vec3{0.418}, glm::vec3{0.28}, glm::vec3{0.37}, glm::vec3{0.64}
  // };
  //
  // std::vector transforms {
  //   instanced_object::model_for(rotations[0], scales_[0], positions[0]),
  //   instanced_object::model_for(rotations[1], scales_[1], positions[1]),
  //   instanced_object::model_for(rotations[2], scales_[2], positions[2]),
  //   instanced_object::model_for(rotations[3], scales_[3], positions[3]),
  //   instanced_object::model_for(rotations[4], scales_[4], positions[4])
  // };
  //
  // const auto obj = cache::construct<render_object>(std::vector<vertex_spec>{
  //   { .position = {-1, 0, -1}, .uvs = {0, 0}, .normal = {0, 1, 0} },
  //   { .position = {1, 0, -1}, .uvs = {1, 0}, .normal = {0, 1, 0} },
  //   { .position = {1, 0, 1}, .uvs = {1, 1}, .normal = {0, 1, 0} },
  //   { .position = {-1, 0, 1}, .uvs = {0, 1}, .normal = {0, 1, 0} },
  // }, std::vector<unsigned int>{ 0, 1, 2, 0, 2, 3 });
  // const auto monkey = cache::load<render_object>("suzanne");
  //
  // const auto sh = cache::load<shader>("perlin", "perlin");
  // const auto phong = cache::load<shader>("phong", "phong");
  // const auto instanced_phong = cache::load<shader>("phong_instanced", "phong_instanced");
  //
  // const auto tex = cache::construct<texture>("plasma");
  //
  // const auto coll = cache::load<collider>("suzanne_collider");
  // const auto inst_coll = cache::load<instanced_collider>("suzanne_collider", transforms);

  auto cam = camera{};

  // float scales[3] = {5.0f, 5.0f, 5.0f};
  //
  // const render_ref perlin_square = cache::construct<renderable>("perlin",
  //   obj, sh, uniforms{0, 1, 2, 3}, std::initializer_list{
  //     std::pair{8u, tex}
  //   }
  // );
  //
  // const render_ref suzanne = cache::construct<renderable>("suzanne",
  //   monkey, phong, uniforms{0, 1, 2, 3}, std::initializer_list{
  //     std::pair{4u, tex}, std::pair{5u, tex}
  //   }
  // );
  // suzanne->position = {0, 0.5, 0};
  // suzanne->scale = {0.5, 0.5, 0.5};
  // suzanne->coll = coll;
  //
  //
  // const auto many_monkey_objects = cache::load<instanced_object>("suzanne", transforms);
  // const instanced_render_ref many_monkeys = cache::construct<instanced_renderable>("monkeys!",
  //   many_monkey_objects, instanced_phong, instanced_uniforms{0, 1}, std::initializer_list{
  //     std::pair{4u, tex}, std::pair{5u, tex}
  //   }, inst_coll
  // );



  // force window initialization etc
  win.frame_pre();
  win.frame_post();

  phong_lighting lights(0.1f, {
    { true, { .pos = {0, 1, 0}, .diffuse = {0.9, 0.9, 0.6} } }
  });
  const auto lighting_suzanne = setup_phong_shading<10>(cam, lights, [](const shader_ref &s, const renderable &r) {
    if (r.coll.has_value()) {
      const unsigned int uniform = s->loc_for("is_highlighted");
      s->set_bool(uniform, (*r.coll)->is_hovered);
    }
  });
  // const auto lighting_monkeys = setup_phong_shading<10, instanced_renderable>(cam, lights, [](const shader_ref &s, const instanced_renderable &r) {
  //   if (r.coll.has_value()) {
  //     const unsigned int uniform = s->loc_for("is_highlighted");
  //     const unsigned int inst_uniform = s->loc_for("highlighted_instance");
  //     s->set_bool(uniform, (*r.coll)->is_hovered);
  //     s->set_uint(inst_uniform, (*r.coll)->highlighted_instance);
  //   }
  // });

  using highlighter = hover_highlighter;

  while (!win.should_close()) {
    if (!win.frame_pre()) continue;

    highlighter::reset();

    cam.handle_input();

    highlighter::highlight_checking(cam);

    if (map.highlight_binding.has_value()) {
      highlighter::bind_highlight_tex(*map.highlight_binding);

      for (const auto &[s, idx] : map.requires_highlight) {
        s->set_int(idx, *map.highlight_binding);
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

    for (const auto &r : map.scene) {
      r->draw(cam, lighting_suzanne);
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