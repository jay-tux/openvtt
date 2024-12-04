#include <iostream>

#include "renderer/window.hpp"
#include "renderer/fps_counter.hpp"
#include "renderer/log_view.hpp"
#include "renderer/renderable.hpp"
#include "renderer/fbo.hpp"

using namespace openvtt::renderer;

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
  const auto highlight = cache::load_shader("basic_mvp", "highlight");

  const auto tex = cache::add_texture("plasma");

  const auto coll = cache::load_collider("suzanne_collider");

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
  suzanne->coll = coll;

  // force window initialization etc
  win.frame_pre();
  win.frame_post();

  const auto win_size_int = [&win] {
    const auto size = win.io_data().DisplaySize;
    log<log_type::INFO>("main", std::format("Got window size: {}x{}", size.x, size.y));
    return std::pair{static_cast<unsigned int>(size.x), static_cast<unsigned int>(size.y)};
  };

  const fbo highlight_fbo{win_size_int()};
  if (!highlight_fbo.verify()) {
    log<log_type::ERROR>("main", "Failed to create highlight FBO");
    return 1;
  }

  phong_lighting lights(0.1f, {
    { true, { .pos = {0, 1, 0}, .diffuse = {0.9, 0.9, 0.6} } }
  });
  const auto lighting_suzanne = setup_phong_shading<10>(cam, lights/*, [&suzanne, &highlight_fbo](const shader_ref &s) {
    static auto h_loc = s->loc_for("highlight_map");
    highlight_fbo.bind_rgb_to(15);
    s->set_int(h_loc, 15);
  }*/);

  std::optional<collider_ref> highlighted = std::nullopt;
  const unsigned int m_highlight = highlight->loc_for("model");
  const unsigned int v_highlight = highlight->loc_for("view");
  const unsigned int p_highlight = highlight->loc_for("projection");

  while (!win.should_close()) {
    if (!win.frame_pre()) continue;

    if (highlighted.has_value()) {
      (*highlighted)->is_hovered = false;
      highlight_fbo.clear();
    }

    cam.handle_input();

    if (const auto hover = cache::mouse_over(cam); hover.has_value()) {
      highlighted = (*hover)->coll; // pre-condition: hover has a collider
      (*highlighted)->is_hovered = true;

      highlight_fbo.with_fbo([&hover, &cam, &highlight, m_highlight, v_highlight, p_highlight] {
        cam.set_matrices(*highlight, v_highlight, p_highlight);
        highlight->set_mat4(m_highlight, (*hover)->model());
        (*hover)->obj->draw(*highlight);
      });
    }

    perlin_square->draw(cam, [&scales](const shader_ref &sr) {
      sr->set_float(4, scales[0]);
      sr->set_float(5, scales[1]);
      sr->set_float(6, scales[2]);
      sr->set_float(7, 0 * static_cast<float>(ImGui::GetTime()));
    });

    highlight_fbo.bind_rgb_to(15);
    phong->set_int(phong->loc_for("highlight_map"), 15);
    suzanne->draw(cam, lighting_suzanne);

    cache::draw_colliders(cam);

    fps_counter::render();
    log_view::render();
    cam.render_controls();
    cache::detail_window();
    lights.detail_window();
    highlight_fbo.draw_texture_imgui("Highlight Buffer", 256, 256);
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