//
// Created by jay on 12/2/24.
//

#ifndef COLLIDER_HPP
#define COLLIDER_HPP

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "camera.hpp"

namespace openvtt::renderer {
/**
 * @brief A struct representing a ray in 3D space.
 */
class ray {
public:
  /**
   * @brief Constructs a ray from an origin and a direction.
   * @param origin The origin of the ray.
   * @param direction The direction of the ray.
   *
   * The direction vector does not need to be normalized (and it isn't normalized in this constructor). However, for
   * numerical stability of the ray-cast algorithm, it is recommended to normalize the direction vector (or not pass
   * direction vectors with a large magnitude).
   */
  ray(const glm::vec3 &origin, const glm::vec3 &direction) : origin{origin}, direction{direction}, inv_direction{1.0f / direction} {}

  /**
   * @brief Gets the origin of the ray.
   * @return The origin of the ray.
   */
  [[nodiscard]] constexpr const glm::vec3 &point() const { return origin; }
  /**
   * @brief Gets the direction of the ray.
   * @return The direction of the ray.
   */
  [[nodiscard]] constexpr const glm::vec3 &dir() const { return direction; }
  /**
   * @brief Gets the inverse of the direction of the ray.
   * @return The inverse of the direction of the ray.
   *
   * The inverse direction is defined as an element-wise inverse of the direction vector. This is a cached value that is
   * used many times in the ray-cast algorithm.
   */
  [[nodiscard]] constexpr const glm::vec3 &inv_dir() const { return inv_direction; }

private:
  glm::vec3 origin; //!< The origin of the ray.
  glm::vec3 direction; //!< The direction of the ray.
  glm::vec3 inv_direction; //!< The (cached) inverse of the direction of the ray.
};

/**
 * @brief Class representing a collider.
 *
 * The collider is a mesh collider based on triangles. To speed up the ray-cast algorithm, the collider also stores the
 * AABB (axis-aligned bounding box) of the mesh.
 */
class collider {
public:
  /**
   * @brief Constructs a new collider.
   * @param vertices The vertices to use.
   * @param indices The indices to use.
   *
   * Just like an OpenGL VBO/EBO pair, the vertices and indices are stored in separate arrays. Each triangle is defined
   * by three consecutive indices, each of which point to a vertex in the vertices array.
   *
   * It is recommended not to use the same vertices and indices as the mesh, but rather create a simplified version of
   * the mesh (for computational efficiency, as the ray-cast algorithm is O(n) in the number of triangles).
   */
  collider(const std::vector<glm::vec3> &vertices, const std::vector<unsigned int> &indices);
  inline collider(const collider &other) : collider(other.vertices, other.indices) {}
  constexpr collider(collider &&other) noexcept {
    std::swap(vertices, other.vertices);
    std::swap(indices, other.indices);
    std::swap(vao, other.vao);
    std::swap(vbo, other.vbo);
    std::swap(ebo, other.ebo);
    std::swap(min, other.min);
    std::swap(max, other.max);
  }
  collider &operator=(const collider &other) = delete;
  collider &operator=(collider &&other) noexcept = delete;

  /**
   * @brief Constructs a new collider from an asset file.
   * @param asset The path to the asset.
   * @return The loaded collider.
   *
   * The asset's path is computed using @ref openvtt::asset_path. Only vertex positions and face indices are loaded from
   * the file, using Assimp if needed to re-triangulate the mesh.
   */
  static collider load_from(const std::string &asset);

  /**
   * @brief Checks if the given ray intersects the collider.
   * @param r The ray to check.
   * @param model The model matrix of the collider.
   * @return The distance to the intersection point, or infinity if there is no intersection.
   *
   * Since the stored vertices are in object space, a model matrix is needed to transform them into world space. The
   * collider's AABB is also transformed (and the AABB of the transformed AABB is used to speed up the ray-cast
   * algorithm).
   *
   * If the ray doesn't intersect the (transformed) AABB, the function returns infinity. Otherwise, it loops over all
   * triangles in the mesh, trying to find the closest intersection point (parametric distance along the ray). If no
   * triangle is intersected by the ray, the function returns infinity.
   */
  [[nodiscard]] float ray_intersect(const ray &r, const glm::mat4 &model) const;

  /**
   * @brief Renders the collider.
   *
   * The shader should already be set up, as this function only performs the actual draw call. The collider is rendered
   * as a wireframe mesh, by setting `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)`. After the draw call, the polygon mode
   * is reset by calling `glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)`.
   */
  void draw() const;

  /**
   * @brief Gets the AABB of the collider.
   * @return A pair of vectors, representing the "minimum" and "maximum" points of the AABB.
   */
  [[nodiscard]] constexpr std::pair<glm::vec3, glm::vec3> aabb() const { return {min, max}; }

  ~collider();

  bool is_hovered = false; //!< Whether the collider is currently hovered by the mouse. This value should be cleared before each frame.
private:
  std::vector<glm::vec3> vertices{}; //!< The vertices of the collider.
  std::vector<unsigned int> indices{}; //!< The indices of the collider.
  unsigned int vao = 0; //!< The VAO of the collider, used for rendering.
  unsigned int vbo = 0; //!< The VBO of the collider, used for rendering.
  unsigned int ebo = 0; //!< The EBO of the collider, used for rendering.
  glm::vec3 min{}; //!< The minimum point of the AABB.
  glm::vec3 max{}; //!< The maximum point of the AABB;
};
}

#endif //COLLIDER_HPP
