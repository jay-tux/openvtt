//
// Created by jay on 11/30/24.
//

#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "shader.hpp"
#include "glm_wrapper.hpp"

namespace openvtt::renderer {
/**
 * @brief Structure representing a vertex.
 */
struct vertex_spec {
  glm::vec3 position; /**< The position of the vertex in 3D space. */
  glm::vec2 uvs;      /**< The texture coordinates of the vertex. */
  glm::vec3 normal;   /**< The normal vector of the vertex. */
};

/**
 * @brief A class representing a (renderable) object.
 */
class render_object {
public:
  /**
   * @brief Construct a new render object.
   *
   * @param vs The vertices of the object.
   * @param index The indices of the object.
   *
   * The data is copied straight to GPU memory (VBO and EBO respectively), so the vectors can be safely destroyed after
   * this call.
   */
  render_object(const std::vector<vertex_spec> &vs, const std::vector<unsigned int> &index);

  /**
   * @brief Draw the object using the given shader.
   *
   * @param s The shader to use.
   *
   * All 3D draw calls should precede any Dear IMGUI calls, so the UI is drawn on top of the 3D scene.
   *
   * This function should not be called on `instanced_object`s, as it will only draw the first instance.
   */
  void draw(const shader &s) const;

  /**
   * @brief Load a render object from a file.
   *
   * @param asset The path to the asset.
   * @return The loaded object.
   *
   * The asset's path is computed using @ref openvtt::asset_path.
   */
  static render_object load_from(const std::string &asset);

  render_object(const render_object &other) = delete;
  constexpr render_object(render_object &&other) noexcept {
    std::swap(vbo, other.vbo);
    std::swap(ebo, other.ebo);
    std::swap(vao, other.vao);
    std::swap(elements, other.elements);
  }
  render_object &operator=(const render_object &other) = delete;
  render_object &operator=(render_object &&other) = delete;

  virtual ~render_object();
protected:
  void bind_vao() const;

  unsigned int vbo = 0; //!< Vertex Buffer Object.
  unsigned int ebo = 0; //!< Element Buffer Object.
  unsigned int vao = 0; //!< Vertex Array Object.
  size_t elements = -1ul; //!< The number of elements in the EBO.
};

/**
 * @brief Class representing an instanced object.
 *
 * All instances of an instanced object can be drawn with a single draw call (where normal objects would require a draw
 * call per instance). However, the model matrix (aka transform) of each instance is fixed and cannot be changed.
 * Additionally, these objects don't support the hovering effect (as they don't support colliders).
 */
class instanced_object final : render_object {
public:
  /**
   * @brief Construct a new instanced object.
   * @param vs The vertices of the object (see render_object::render_object).
   * @param index The indices of the object (see render_object::render_object).
   * @param models The model matrices of the instances.
   *
   * The vertices and indices are passed straight through to the render_object constructor. The model matrices are copied
   * by the constructor and moved to the GPU.
   */
  inline instanced_object(const std::vector<vertex_spec> &vs, const std::vector<unsigned int> &index, const std::vector<glm::mat4> &models)
    : instanced_object(std::move(render_object(vs, index)), models) {}
  constexpr instanced_object(instanced_object &&other) noexcept : render_object(std::move(other)) {
    std::swap(model_vbo, other.model_vbo);
    std::swap(instances, other.instances);
  }

  /**
   * @brief Draw all instances of the object using the given shader.
   * @param s The shader to use.
   *
   * Using the `draw` method of the base class will result in only the first of the instances being drawn.
   */
  void draw_instanced(const shader &s) const;

  /**
   * @brief Load a render object from a file, and duplicate it multiple times.
   * @param asset The path to the asset.
   * @param models The model matrices of the instances.
   * @return The loaded object.
   *
   * The asset's path is computed using @ref openvtt::asset_path.
   */
  inline static instanced_object load_from(const std::string &asset, const std::vector<glm::mat4> &models) {
    return {render_object::load_from(asset), models};
  }

  /**
   * @brief Create a model matrix for a given position, scale, and yaw-pitch-roll angles.
   * @param ypr The yaw-pitch-roll angles.
   * @param scale The scale of the object.
   * @param pos The position of the object.
   *
   * The transform of the object is applied in the following order:
   * 1. Rotate around the Y-axis (yaw).
   * 2. Rotate around the X-axis (pitch).
   * 3. Rotate around the Z-axis (roll).
   * 4. Scale.
   * 5. Translate.
   */
  constexpr static glm::mat4 model_for(const glm::vec3 &ypr, const glm::vec3 &scale, const glm::vec3 &pos) {
    return glm::mat4(1.0f) | translation(pos) | rescale(scale) | roll(ypr.z) | pitch(ypr.x) | yaw(ypr.y);
  }

  ~instanced_object() override;
private:
  instanced_object(render_object &&ro, const std::vector<glm::mat4> &models);
  unsigned int model_vbo = 0; //!< The additional VBO for the model matrices.
  unsigned int model_inv_t_vbo = 0; //!< The additional VBO for the inverse-transpose of the model matrices.
  size_t instances = -1ul; //!< The number of instances.
};
}

#endif //OBJECT_HPP
