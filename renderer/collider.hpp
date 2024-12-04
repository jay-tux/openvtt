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
class ray {
public:
  ray(const glm::vec3 &origin, const glm::vec3 &direction) : origin{origin}, direction{direction}, inv_direction{1.0f / direction} {}

  [[nodiscard]] constexpr const glm::vec3 &point() const { return origin; }
  [[nodiscard]] constexpr const glm::vec3 &dir() const { return direction; }
  [[nodiscard]] constexpr const glm::vec3 &inv_dir() const { return inv_direction; }

private:
  glm::vec3 origin;
  glm::vec3 direction;
  glm::vec3 inv_direction;
};

class collider {
public:
  collider(const std::vector<glm::vec3> &vertices, const std::vector<unsigned int> &indices);
  collider(const collider &other) = delete;
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

  static collider load_from(const std::string &asset);

  [[nodiscard]] float ray_intersect(const ray &r, const glm::mat4 &model) const;

  void draw() const;

  ~collider();

  bool is_hovered = false;
private:
  std::vector<glm::vec3> vertices{};
  std::vector<unsigned int> indices{};
  unsigned int vao = 0;
  unsigned int vbo = 0;
  unsigned int ebo = 0;
  glm::vec3 min{};
  glm::vec3 max{};
};
}

#endif //COLLIDER_HPP
