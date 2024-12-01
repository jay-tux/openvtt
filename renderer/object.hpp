//
// Created by jay on 11/30/24.
//

#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "shader.hpp"

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

  ~render_object();
private:
  unsigned int vbo = 0; //!< Vertex Buffer Object.
  unsigned int ebo = 0; //!< Element Buffer Object.
  unsigned int vao = 0; //!< Vertex Array Object.
  size_t elements = -1ul; //!< The number of elements in the EBO.
};
}

#endif //OBJECT_HPP
