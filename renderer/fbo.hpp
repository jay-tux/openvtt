//
// Created by jay on 12/4/24.
//

#ifndef HIGHLIGHT_FBO_HPP
#define HIGHLIGHT_FBO_HPP

#include <utility>

namespace openvtt::renderer {
class fbo {
public:
  explicit inline fbo(const std::pair<unsigned int, unsigned int> &size) : fbo(size.first, size.second) {}
  fbo(unsigned int w, unsigned int h);
  fbo(const fbo &other) = delete;
  fbo(fbo &&other) noexcept = delete;
  fbo &operator=(const fbo &other) = delete;
  fbo &operator=(fbo &&other) noexcept = delete;

  void bind() const;
  void clear() const;
  void unbind() const;
  [[nodiscard]] constexpr unsigned int rgb() const { return rgb_tex; }
  void bind_rgb_to(unsigned int slot) const;
  [[nodiscard]] bool verify() const;

  void resize(int nw, int nh);
  inline void resize(const std::pair<int, int> &size) { resize(size.first, size.second); }

  template <std::invocable F>
  void with_fbo(F &&f) const {
    bind();
    f();
    unbind();
  }

  void draw_texture_imgui(const char *name, int w, int h) const;

  ~fbo();
private:
  unsigned int fbo_id = 0;
  unsigned int rgb_tex = 0;
  unsigned int w;
  unsigned int h;
  mutable int vp_cache[4]{};
};
}

#endif //HIGHLIGHT_FBO_HPP
