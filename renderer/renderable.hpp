//
// Created by jay on 11/30/24.
//

#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <optional>
#include <utility>

#include "glm_wrapper.hpp"
#include "log_view.hpp"

namespace openvtt::renderer {
struct renderable;
}

#include "render_cache.hpp"
#include "camera.hpp"

namespace openvtt::renderer {
/**
 * @brief A struct to hold the required uniform locations for a shader (3D).
 */
struct uniforms {
  unsigned int model; //!< The location of the model matrix uniform.
  unsigned int view; //!< The location of the view matrix uniform.
  unsigned int projection;  //!< The location of the projection matrix uniform.
  unsigned int model_inv_t; //!< The location of the uniform for the inverse-transpose of the model matrix.

  /**
   * @brief Create a uniforms struct from a shader.
   * @param s The shader to get the uniform locations from.
   * @return A uniforms struct with the locations of the required uniforms.
   *
   * The uniforms are dynamically loaded from the shader. This forces the shader to have the following uniforms:
   * - `model` (mat4): The model matrix.
   * - `view` (mat4): The view matrix.
   * - `projection` (mat4): The projection matrix.
   * - `model_inv_t` (mat3): The inverse-transpose of the model matrix.
   */
  inline static uniforms from_shader(const shader_ref &s) {
    return {
      .model = s->loc_for("model"),
      .view = s->loc_for("view"),
      .projection = s->loc_for("projection"),
      .model_inv_t = s->loc_for("model_inv_t")
    };
  }
};

/**
 * @brief A struct to hold a renderable.
 *
 * A renderable is an object, together with its shader and textures, that can be drawn to the screen. It also features
 * a rudimentary transform (position, rotation (using yaw-pitch-roll), and scale), and a name.
 *
 * Each of the fields in this struct can be edited at will. Use the `draw` function to draw the renderable to the
 * screen.
 */
struct renderable {
  /**
   * @brief Construct a renderable.
   * @param name The name of the renderable.
   * @param o The object to render.
   * @param s The shader to use.
   * @param uniforms The uniforms struct for the shader.
   * @param ts A list of pairs of texture locations and textures.
   *
   * Each of the pairs (loc, tex) in `ts` should be a location in the shader, along with which texture to bind to that
   * location.
   */
  inline renderable(
    std::string name,
    const object_ref &o,
    const shader_ref &s,
    const uniforms &uniforms,
    const std::initializer_list<std::pair<unsigned int, texture_ref>> ts
  ) : obj{o}, sh{s}, textures{ts}, name{std::move(name)}, model_loc{uniforms.model}, view_loc{uniforms.view}, proj_loc{uniforms.projection},
      model_inv_t_loc{uniforms.model_inv_t} {}

  inline renderable(
    std::string name,
    const object_ref &o,
    const shader_ref &s,
    const uniforms &uniforms,
    const std::vector<std::pair<unsigned int, texture_ref>> &ts
  ) : obj{o}, sh{s}, textures{ts}, name{std::move(name)}, model_loc{uniforms.model}, view_loc{uniforms.view}, proj_loc{uniforms.projection},
      model_inv_t_loc{uniforms.model_inv_t} {}

  [[nodiscard]] inline glm::mat4 model() const {
    return glm::mat4(1.0f) | translation(position) | rescale(scale) | roll(rotation.z) | pitch(rotation.x) | yaw(rotation.y);
  }

  /**
   * @brief Draw the renderable.
   * @param cam The camera to use for drawing.
   *
   * This is a convenience function that calls `draw(cam, [](...){})`.
   */
  inline void draw(const camera &cam) const {
    draw(cam, [](const auto &, const auto &){});
  }

  /**
   * @brief Draw the renderable.
   * @tparam F A callable type `(const shader_ref &, const renderable &) -> void`.
   * @param cam The camera to use for drawing.
   * @param f A function to perform additional shader setup.
   *
   * If the shader is disabled (`active == false`), this function is a no-op.
   *
   * This function activates the shader, then sets all required base uniforms (model, view, projection,
   * inverse-transpose of the model). It then calls `f` with the shader, allowing for additional setup. Finally, it
   * performs the actual rendering of the object.
   *
   * If the shader supports Phong shading, you can use `setup_phong_shading` to set up the shader for Phong shading.
   */
  template <std::invocable<const shader_ref &, const renderable &> F>
  inline void draw(const camera &cam, F &&f) const {
    if (!active) return;

    sh->activate();

    const auto m = model();
    sh->set_mat4(model_loc, m);
    cam.set_matrices(*sh, view_loc, proj_loc);
    sh->set_mat3(model_inv_t_loc, glm::mat3(transpose(inverse(m))));
    f(sh, *this);
    int i = 0;
    for (const auto &[loc, tex] : textures) {
      tex->bind(i);
      sh->set_int(loc, i);
      ++i;
    }
    obj->draw(*sh);
  }

  object_ref obj; //!< The object to render.
  shader_ref sh; //!< The shader to use.
  std::vector<std::pair<unsigned int, texture_ref>> textures; //!< The textures to use.

  bool active = true; //!< Whether the renderable is active (i.e. should be rendered).
  std::string name; //!< The name of the renderable.
  glm::vec3 position{0,0,0}; //!< The position of the renderable.
  glm::vec3 rotation{0, 0, 0}; //!< The rotation of the renderable.
  glm::vec3 scale{1,1,1}; //!< The scale of the renderable.

  std::optional<collider_ref> coll = std::nullopt;

  unsigned int model_loc; //!< The location of the model matrix uniform.
  unsigned int view_loc; //!< The location of the view matrix uniform.
  unsigned int proj_loc; //!< The location of the projection matrix uniform.
  unsigned int model_inv_t_loc; //!< The location of the uniform for the inverse-transpose of the model matrix.
};

/**
 * @brief A struct to represent the uniforms required for Phong shading.
 * @tparam point_light_count The number of point lights to support.
 */
template <size_t point_light_count>
struct phong_uniforms {
  /**
   * @brief A struct to represent a point light.
   */
  struct point_light {
    unsigned int pos; //!< The location of the position uniform for this point light (vec3).
    unsigned int diffuse; //!< The location of the diffuse light color uniform for this point light (vec3).
    unsigned int specular; //!< The location of the specular light color uniform for this point light (vec3).
    unsigned int attenuation; //!< The location of the attenuation uniform for this point light (vec3: constant, linear, and quadratic).
  };

  unsigned int view_pos; //!< The location of the view position uniform (vec3).
  unsigned int ambient_light; //!< The location of the ambient light strength uniform (float).
  unsigned int used_point_count; //!< The location of the number of point lights used uniform (int).
  point_light points[point_light_count]; //!< The point lights.

  /**
   * @brief Create a phong_uniforms struct from a shader.
   * @param s The shader to get the uniform locations from.
   * @return A phong_uniforms struct with the locations of the required uniforms.
   *
   * The uniforms are dynamically loaded from the shader. This forces the shader to have the following uniforms:
   * - `view_pos` (vec3): The view position.
   * - `ambient_light` (float): The ambient light strength.
   * - `used_point_count` (int): The number of point lights used.
   * - `points` (array of size `point_light_count`): The point lights.
   *
   * The point lights should be a struct with at least the following fields:
   * - `points[i].pos` (vec3): The position of the `i`th point light.
   * - `points[i].diffuse` (vec3): The diffuse light color of the `i`th point light.
   * - `points[i].specular` (vec3): The specular light color of the `i`th point light.
   * - `points[i].attenuation` (vec3): The attenuation of the `i`th point light (constant, linear, quadratic).
   */
  static phong_uniforms from_shader(const shader_ref &s) {
    phong_uniforms res;
    res.view_pos = s->loc_for("view_pos");
    res.ambient_light = s->loc_for("ambient_light");
    res.used_point_count = s->loc_for("used_point_count");
    for (size_t i = 0; i < point_light_count; ++i) {
      res.points[i] = {
        .pos = s->loc_for("points[" + std::to_string(i) + "].pos"),
        .diffuse = s->loc_for("points[" + std::to_string(i) + "].diffuse"),
        .specular = s->loc_for("points[" + std::to_string(i) + "].specular"),
        .attenuation = s->loc_for("points[" + std::to_string(i) + "].attenuation")
      };
    }
    return res;
  }
};

/**
 * @brief A struct to represent Phong lighting.
 */
class phong_lighting {
public:
  /**
   * @brief A struct to represent a point light.
   */
  struct point_light {
    glm::vec3 pos; //!< The position of the point light.
    glm::vec3 diffuse; //!< The diffuse light color of the point light.
    glm::vec3 specular{1, 1, 1}; //!< The specular light color of the point light.
    glm::vec3 attenuation {1.0f, 0.09f, 0.032f}; //!< The attenuation of the point light (constant, linear, quadratic).
  };

  /**
   * @brief Construct a phong_lighting struct.
   * @param ambient The ambient light strength.
   * @param points A list of point lights.
   *
   * Each of the point lights in `points` should be a pair (active, light), where `active` is a boolean indicating
   * whether the light is active, and `light` is the point light.
   */
  explicit constexpr phong_lighting(
    const float ambient = 0.1f,
    const std::initializer_list<std::pair<bool, point_light>> points = {}
  ) : ambient_strength{ambient}, points{points} {}

  /**
   * @brief Get the number of point lights.
   * @return The number of point lights.
   */
  [[nodiscard]] constexpr size_t size() const { return points.size(); }
  /**
   * @brief Get a (const reference to a) point light.
   * @param i The index of the point light.
   * @return A pair (active, light) where `active` is a boolean indicating whether the light is active, and `light` is
   * the point light.
   */
  [[nodiscard]] constexpr const std::pair<bool, point_light> &operator[](const size_t i) const { return points[i]; }
  /**
   * @brief Get a (reference to a) point light.
   * @param i The index of the point light.
   * @return A pair (active, light) where `active` is a boolean indicating whether the light is active, and `light` is
   * the point light.
   */
  [[nodiscard]] constexpr std::pair<bool, point_light> &operator[](const size_t i) { return points[i]; }

  /**
   * @brief Add a point light.
   * @param on Whether the point light is active.
   * @param pt The point light to add.
   */
  void add_point(const bool on, const point_light &pt) {
    points.emplace_back(on, pt);
  }

  /**
   * @brief Draw the detail window for the phong lighting.
   *
   * This window shows the ambient light strength, as well as all details for each point light. It allows for editing
   * each of these values (including the enabled-ness of each point light). Additionally, it allows for adding new
   * point lights.
   */
  void detail_window();

  float ambient_strength; //!< The ambient light strength.
  std::vector<std::pair<bool, point_light>> points; //!< The point lights.
};

/**
 * @brief Sets up Phong shading for a shader.
 *
 * @tparam point_light_count The number of point lights to support.
 * @tparam F A callable type `(const shader_ref &) -> void`.
 * @param cam The camera to use for setting up the shader.
 * @param lighting The Phong lighting settings.
 * @param f A function to perform additional shader setup.
 * @return A callable that sets up the Phong shading uniforms in the shader.
 *
 * This function template sets up the necessary uniforms for Phong shading in a shader. It configures the view position,
 * ambient light strength, and point lights based on the provided camera and lighting settings. It also allows for
 * additional shader setup through a provided callable.
 */
template <size_t point_light_count, std::invocable<const shader_ref &, const renderable &> F>
constexpr std::invocable<const shader_ref &, const renderable &> auto setup_phong_shading(
  camera &cam, phong_lighting &lighting, F &&f
) {
  return [&f, &cam, &lighting](const shader_ref &sr, const renderable &r) {
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

    f(sr, r);
  };
}

/**
 * @brief Sets up Phong shading for a shader.
 *
 * @tparam point_light_count The number of point lights to support.
 * @param cam The camera to use for setting up the shader.
 * @param lighting The Phong lighting settings.
 * @return A callable that sets up the Phong shading uniforms in the shader.
 *
 * This function template sets up the necessary uniforms for Phong shading in a shader. It configures the view position,
 * ambient light strength, and point lights based on the provided camera and lighting settings.
 */
template <size_t point_light_count>
constexpr std::invocable<const shader_ref &, const renderable &> auto setup_phong_shading(
  camera &cam, phong_lighting &lighting
) {
  return setup_phong_shading<point_light_count>(cam, lighting, [](const auto &, const auto &){});
}
}

#endif //RENDERABLE_HPP
