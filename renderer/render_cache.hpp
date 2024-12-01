//
// Created by jay on 11/30/24.
//

#ifndef RENDER_CACHE_HPP
#define RENDER_CACHE_HPP

#include <vector>

#include "window.hpp"
#include "object.hpp"
#include "shader.hpp"
#include "texture.hpp"

namespace gltt::renderer {
class object_ref;
class shader_ref;
class texture_ref;
class render_ref;
class render_cache;

constexpr render_object &operator*(const object_ref &r);
constexpr shader &operator*(const shader_ref &r);
constexpr texture &operator*(const texture_ref &r);

class object_ref {
public:
  constexpr render_object *operator->() const { return &**this; }
private:
  constexpr explicit object_ref(const size_t idx) : idx{idx} {}
  size_t idx;
  friend class render_cache;
};

class shader_ref {
public:
  constexpr shader *operator->() const { return &**this; }
private:
  constexpr explicit shader_ref(const size_t idx) : idx{idx} {}
  size_t idx;
  friend class render_cache;
};

class texture_ref {
public:
  constexpr texture *operator->() const { return &**this; }
private:
  constexpr explicit texture_ref(const size_t idx) : idx{idx} {}
  size_t idx;
  friend class render_cache;
};
}

#include "renderable.hpp"

namespace gltt::renderer {
constexpr renderable &operator*(const render_ref &r);

class render_ref {
public:
  constexpr renderable *operator->() const { return &**this; }
private:
  constexpr explicit render_ref(const size_t idx) : idx{idx} {}
  size_t idx;
  friend class render_cache;
};

class render_cache {
public:
  constexpr render_cache() = default;

  constexpr static render_object &operator[](const object_ref &r) { return objects[r.idx]; }
  constexpr static shader &operator[](const shader_ref &r) { return shaders[r.idx]; }
  constexpr static texture &operator[](const texture_ref &r) { return textures[r.idx]; }
  constexpr static renderable &operator[](const render_ref &r) { return renderables[r.idx]; }

  constexpr static object_ref load_object(const std::string &asset) {
    objects.emplace_back(render_object::load_from(asset));
    return object_ref{objects.size() - 1};
  }

  template <typename ... Ts> requires(std::constructible_from<render_object, Ts...>)
  constexpr static object_ref add_object(Ts &&... ts) {
    objects.emplace_back(std::forward<Ts>(ts)...);
    return object_ref{objects.size() - 1};
  }

  constexpr static shader_ref load_shader(const std::string &vs, const std::string &fs) {
    shaders.emplace_back(shader::from_assets(vs, fs));
    return shader_ref{shaders.size() - 1};
  }

  template <typename ... Ts> requires(std::constructible_from<shader, Ts...>)
  constexpr static shader_ref add_shader(Ts &&... ts) {
    shaders.emplace_back(std::forward<Ts>(ts)...);
    return shader_ref{shaders.size() - 1};
  }

  template <typename ... Ts> requires(std::constructible_from<texture, Ts...>)
  constexpr static texture_ref add_texture(Ts &&... ts) {
    textures.emplace_back(std::forward<Ts>(ts)...);
    return texture_ref{textures.size() - 1};
  }

  template <typename ... Ts> requires(std::constructible_from<renderable, Ts...>)
  constexpr static render_ref add_renderable(Ts &&... ts) {
    renderables.emplace_back(std::forward<Ts>(ts)...);
    return render_ref{renderables.size() - 1};
  }

  static void detail_window();

private:
  static inline std::vector<render_object> objects{};
  static inline std::vector<shader> shaders{};
  static inline std::vector<texture> textures{};
  static inline std::vector<renderable> renderables{};
};

constexpr static inline auto cache = render_cache{};

constexpr render_object &operator*(const object_ref &r) { return cache[r]; }
constexpr shader &operator*(const shader_ref &r) { return cache[r]; }
constexpr texture &operator*(const texture_ref &r) { return cache[r]; }
constexpr renderable &operator*(const render_ref &r) { return cache[r]; }
}

#endif //RENDER_CACHE_HPP
