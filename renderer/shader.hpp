//
// Created by jay on 11/30/24.
//

#ifndef SHADER_HPP
#define SHADER_HPP

#include <string>
#include <glm/glm.hpp>

namespace gltt::renderer {
class shader {
public:
  shader(const std::string &vs, const std::string &fs);

  shader(const shader &other) = delete;
  constexpr shader(shader &&other) noexcept {
    std::swap(program, other.program);
  }
  shader &operator=(const shader &other) = delete;
  shader &operator=(shader &&other) = delete;

  static shader from_assets(const std::string &vsf, const std::string &fsf);

  [[nodiscard]] unsigned int loc_for(const std::string &name) const;

  void set_int(unsigned int loc, int i) const;
  void set_float(unsigned int loc, float f) const;
  void set_vec2(unsigned int loc, const glm::vec2 &v) const;
  void set_vec3(unsigned int loc, const glm::vec3 &v) const;
  void set_vec4(unsigned int loc, const glm::vec4 &v) const;
  void set_mat3(unsigned int loc, const glm::mat3 &m) const;
  void set_mat4(unsigned int loc, const glm::mat4 &m) const;

  void activate() const;

  ~shader();
private:
  unsigned int program = 0;
};
}

#endif //SHADER_HPP
