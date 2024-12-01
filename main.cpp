#include <iostream>

#include "renderer/window.hpp"
#include "renderer/fps_counter.hpp"
#include "renderer/log_view.hpp"
#include "renderer/renderable.hpp"

using namespace gltt::renderer;

int main(int argc, const char **argv) {
  using cache = render_cache;
  auto &win = window::get();

  const auto obj = cache::add_object(std::vector<vertex_spec>{
    { .position = {-1, 0, -1}, .uvs = {0, 0}, .normal = {0, 1, 0} },
    { .position = {1, 0, -1}, .uvs = {1, 0}, .normal = {0, 1, 0} },
    { .position = {1, 0, 1}, .uvs = {1, 1}, .normal = {0, 1, 0} },
    { .position = {-1, 0, 1}, .uvs = {0, 1}, .normal = {0, 1, 0} },
  }, std::vector<unsigned int>{ 0, 1, 2, 0, 2, 3 });
  const auto monkey = cache::load_object("suzanne");
  const auto sh = cache::load_shader("perlin", "perlin");
  const auto phong = cache::load_shader("phong", "phong");
  const auto tex = cache::add_texture("plasma");
  auto cam = camera{};

  float scales[3] = {5.0f, 5.0f, 5.0f};

  const render_ref perlin_square = cache::add_renderable("perlin",
    obj, sh, uniforms{0, 1, 2, 3}, std::initializer_list{
      std::pair{8u, tex}
    }
  );

  const render_ref suzanne = cache::add_renderable("suzanne",
    monkey, phong, uniforms{0, 1, 2, 3}, std::initializer_list{
      std::pair{4u, tex}, std::pair{5u, tex}
    }
  );
  suzanne->position = {0, 0.5, 0};
  suzanne->scale = {0.5, 0.5, 0.5};

  phong_lighting lights(0.1f, {
    { true, { .pos = {0, 1, 0}, .diffuse = {0.9, 0.9, 0.6} } }
  });

  const auto lighting_suzanne = setup_phong_shading<10>(cam, lights);

  while (!win.should_close()) {
    if (!win.frame_pre()) continue;

    cam.handle_input();

    perlin_square->draw(cam, [&scales](const shader_ref &sr) {
      sr->set_float(4, scales[0]);
      sr->set_float(5, scales[1]);
      sr->set_float(6, scales[2]);
      sr->set_float(7, 0 * static_cast<float>(ImGui::GetTime()));
    });
    suzanne->draw(cam, lighting_suzanne);

    fps_counter::render();
    log_view::render();
    cam.render_controls();
    cache::detail_window();
    lights.detail_window();
    // ImGui::ShowMetricsWindow();

    ImGui::Begin("Perlin Parameters");
    ImGui::SliderFloat("scale r", scales, -5.0f, 50.0f);
    ImGui::SliderFloat("scale g", scales + 1, -5.0f, 50.0f);
    ImGui::SliderFloat("scale b", scales + 2, -5.0f, 50.0f);
    ImGui::End();

    win.frame_post();
  }

  return 0;
}