//
// Created by jay on 11/30/24.
//

#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <unordered_map>
#include <utility>

#include "log_view.hpp"

namespace gltt::renderer {
struct renderable;
}

#include "render_cache.hpp"
#include "camera.hpp"

namespace gltt::renderer {
struct uniforms {
  unsigned int model;
  unsigned int view;
  unsigned int projection;
  unsigned int model_inv_t;

  inline static uniforms from_shader(const shader_ref &s) {
    return {
      .model = s->loc_for("model"),
      .view = s->loc_for("view"),
      .projection = s->loc_for("projection"),
      .model_inv_t = s->loc_for("model_inv_t")
    };
  }
};

struct renderable {
  inline renderable(
    std::string name,
    const object_ref &o,
    const shader_ref &s,
    const uniforms &uniforms,
    const std::initializer_list<std::pair<unsigned int, texture_ref>> ts
  ) : obj{o}, sh{s}, textures{ts}, name{std::move(name)}, model_loc{uniforms.model}, view_loc{uniforms.view}, proj_loc{uniforms.projection},
      model_inv_t_loc{uniforms.model_inv_t} {}

  inline void draw(const camera &cam) const {
    draw(cam, [](const shader_ref &){});
  }

  template <std::invocable<const shader_ref &> F>
  inline void draw(const camera &cam, F &&f) const {
    if (!active) return;

    sh->activate();
    const glm::mat4 model = translate(
      glm::scale(
        glm::yawPitchRoll(rotation.x, rotation.y, rotation.z),
        scale),
      position
    );

    sh->set_mat4(model_loc, model);
    cam.set_matrices(*sh, view_loc, proj_loc);
    sh->set_mat3(model_inv_t_loc, glm::mat3(transpose(inverse(model))));
    f(sh);
    int i = 0;
    for (const auto &[loc, tex] : textures) {
      tex->bind(i);
      sh->set_int(loc, i);
      ++i;
    }
    obj->draw(*sh);
  }

  object_ref obj;
  shader_ref sh;
  std::vector<std::pair<unsigned int, texture_ref>> textures;

  bool active = true;
  std::string name;
  glm::vec3 position{0,0,0};
  glm::vec3 rotation{0, 0, 0};
  glm::vec3 scale{1,1,1};

  unsigned int model_loc;
  unsigned int view_loc;
  unsigned int proj_loc;
  unsigned int model_inv_t_loc;
};

template <size_t point_light_count>
struct phong_uniforms {
  struct point_light {
    unsigned int pos;
    unsigned int ambient;
    unsigned int diffuse;
    unsigned int specular;
    unsigned int attenuation;
  };

  unsigned int view_pos;
  unsigned int ambient_light;
  unsigned int used_point_count;
  point_light points[point_light_count];

  static phong_uniforms from_shader(const shader_ref &s) {
    phong_uniforms res;
    res.view_pos = s->loc_for("view_pos");
    res.ambient_light = s->loc_for("ambient_light");
    res.used_point_count = s->loc_for("used_point_count");
    for (size_t i = 0; i < point_light_count; ++i) {
      res.points[i] = {
        .pos = s->loc_for("points[" + std::to_string(i) + "].pos"),
        .ambient = s->loc_for("points[" + std::to_string(i) + "].ambient"),
        .diffuse = s->loc_for("points[" + std::to_string(i) + "].diffuse"),
        .specular = s->loc_for("points[" + std::to_string(i) + "].specular"),
        .attenuation = s->loc_for("points[" + std::to_string(i) + "].attenuation")
      };
    }
    return res;
  }
};

class phong_lighting {
public:
  struct point_light {
    glm::vec3 pos;
    glm::vec3 diffuse;
    glm::vec3 specular{1, 1, 1};
    glm::vec3 attenuation {1.0f, 0.09f, 0.032f};
  };

  explicit constexpr phong_lighting(
    const float ambient = 0.1f,
    const std::initializer_list<std::pair<bool, point_light>> points = {}
  ) : ambient_strength{ambient}, points{points} {}

  [[nodiscard]] constexpr size_t size() const { return points.size(); }
  constexpr const std::pair<bool, point_light> &operator[](const size_t i) const { return points[i]; }
  constexpr std::pair<bool, point_light> &operator[](const size_t i) { return points[i]; }

  void add_point(const bool on, const point_light &pt) {
    points.emplace_back(on, pt);
  }

  void detail_window();

  float ambient_strength;
  std::vector<std::pair<bool, point_light>> points;
};

template <size_t point_light_count, std::invocable<const shader_ref &> F>
constexpr std::invocable<const shader_ref &> auto setup_phong_shading(
  camera &cam, phong_lighting &lighting, F &&f
) {
  return [&f, &cam, &lighting](const shader_ref &sr) {
    static auto uniforms = phong_uniforms<point_light_count>::from_shader(sr);

    sr->set_vec3(uniforms.view_pos, cam.position);
    sr->set_float(uniforms.ambient_light, lighting.ambient_strength);
    int used = 0;
    for (size_t i = 0; i < lighting.size() && used < point_light_count; i++) {
      auto [active, light] = lighting[i];
      if (!active) continue;
      sr->set_vec3(uniforms.points[used].pos, light.pos);
      sr->set_vec3(uniforms.points[used].diffuse, light.diffuse);
      sr->set_vec3(uniforms.points[used].specular, light.specular);
      sr->set_vec3(uniforms.points[used].attenuation, light.attenuation);
      ++used;
    }
    sr->set_int(uniforms.used_point_count, used);

    f(sr);
  };
}

template <size_t point_light_count>
constexpr std::invocable<const shader_ref &> auto setup_phong_shading(
  camera &cam, phong_lighting &lighting
) {
  return setup_phong_shading<point_light_count>(cam, lighting, [](const shader_ref &){});
}
}

#endif //RENDERABLE_HPP
