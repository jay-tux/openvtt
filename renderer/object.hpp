//
// Created by jay on 11/30/24.
//

#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "shader.hpp"

namespace gltt::renderer {
struct vertex_spec {
  glm::vec3 position;
  glm::vec2 uvs;
  glm::vec3 normal;
};

class render_object {
public:
  render_object(const std::vector<vertex_spec> &vs, const std::vector<unsigned int> &index);

  void draw(const shader &s) const;

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
  unsigned int vbo = 0;
  unsigned int ebo = 0;
  unsigned int vao = 0;
  size_t elements;
};
}

#endif //OBJECT_HPP
