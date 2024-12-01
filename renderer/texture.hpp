//
// Created by jay on 11/30/24.
//

#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <string>

namespace gltt::renderer {
class texture {
public:
  explicit texture(const std::string &asset);
  texture(const texture &other) = delete;
  constexpr texture(texture &&other) noexcept {
    std::swap(id, other.id);
  }
  texture &operator=(const texture &other) = delete;
  texture &operator=(texture &&other) = delete;

  void bind(unsigned int slot) const;

  ~texture();
private:
  unsigned int id = 0;
};
}

#endif //TEXTURE_HPP
