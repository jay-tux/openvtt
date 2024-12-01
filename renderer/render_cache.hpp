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

namespace openvtt::renderer {
class object_ref;
class shader_ref;
class texture_ref;
class render_ref;
class render_cache;

constexpr render_object &operator*(const object_ref &r);
constexpr shader &operator*(const shader_ref &r);
constexpr texture &operator*(const texture_ref &r);

/**
 * @brief A reference to a render object.
 *
 * These references are indexes into the render cache's object list.
 */
class object_ref {
public:
  /**
   * @brief Dereference the object reference.
   * @return (A pointer to) the object the reference points to.
   *
   * This function allows the object reference to be used as a pointer to the object it references.
   */
  constexpr render_object *operator->() const { return &**this; }
private:
  constexpr explicit object_ref(const size_t idx) : idx{idx} {}
  size_t idx; //!< The index of the object in the render cache.
  friend class render_cache;
};

/**
 * @brief A reference to a shader.
 *
 * These references are indexes into the render cache's shader list.
 */
class shader_ref {
public:
  /**
   * @brief Dereference the shader reference.
   * @return (A pointer to) the shader the reference points to.
   *
   * This function allows the shader reference to be used as a pointer to the shader it references.
   */
  constexpr shader *operator->() const { return &**this; }
private:
  constexpr explicit shader_ref(const size_t idx) : idx{idx} {}
  size_t idx; //!< The index of the shader in the render cache.
  friend class render_cache;
};

/**
 * @brief A reference to a texture.
 *
 * These references are indexes into the render cache's texture list.
 */
class texture_ref {
public:
  /**
   * @brief Dereference the texture reference.
   * @return (A pointer to) the texture the reference points to.
   *
   * This function allows the texture reference to be used as a pointer to the texture it references.
   */
  constexpr texture *operator->() const { return &**this; }
private:
  constexpr explicit texture_ref(const size_t idx) : idx{idx} {}
  size_t idx; //!< The index of the texture in the render cache.
  friend class render_cache;
};
}

#include "renderable.hpp"

namespace openvtt::renderer {
constexpr renderable &operator*(const render_ref &r);

/**
 * @brief A reference to a renderable.
 *
 * These references are indexes into the render cache's renderable list.
 */
class render_ref {
public:
  /**
   * @brief Dereference the renderable reference.
   * @return (A pointer to) the renderable the reference points to.
   *
   * This function allows the renderable reference to be used as a pointer to the renderable it references.
   */
  constexpr renderable *operator->() const { return &**this; }
private:
  constexpr explicit render_ref(const size_t idx) : idx{idx} {}
  size_t idx; //!< The index of the renderable in the render cache.
  friend class render_cache;
};

/**
 * @brief A cache of render objects, shaders, textures, and renderables.
 *
 * Each of the types is stored contiguously in memory, and can be accessed using a reference to the cache. Pointers and
 * references to any object, shader, texture, or renderable can be invalidated if the cache is modified, so it is
 * recommended to not store them. Use the references from the cache instead.
 */
class render_cache {
public:
  constexpr render_cache() = default;

  /**
   * @brief Get a render object from the cache.
   * @param r The reference to the object.
   * @return The object the reference points to.
   */
  constexpr static render_object &operator[](const object_ref &r) { return objects[r.idx]; }
  /**
   * @brief Get a shader from the cache.
   * @param r The reference to the shader.
   * @return The shader the reference points to.
   */
  constexpr static shader &operator[](const shader_ref &r) { return shaders[r.idx]; }
  /**
   * @brief Get a texture from the cache.
   * @param r The reference to the texture.
   * @return The texture the reference points to.
   */
  constexpr static texture &operator[](const texture_ref &r) { return textures[r.idx]; }
  /**
   * @brief Get a renderable from the cache.
   * @param r The reference to the renderable.
   * @return The renderable the reference points to.
   */
  constexpr static renderable &operator[](const render_ref &r) { return renderables[r.idx]; }

  /**
   * @brief Load an object from an asset, and add it to the cache.
   * @param asset The path to the asset.
   * @return A reference to the object.
   *
   * The object is loaded using the @ref `render_object::load_from` function, and then added to the cache.
   */
  constexpr static object_ref load_object(const std::string &asset) {
    objects.emplace_back(render_object::load_from(asset));
    return object_ref{objects.size() - 1};
  }

  /**
   * @brief Add an object to the cache.
   * @tparam Ts The types of the arguments to the object's constructor.
   * @param ts The arguments to the object's constructor.
   * @return A reference to the object.
   *
   * The object is constructed in-place in the cache.
   */
  template <typename ... Ts> requires(std::constructible_from<render_object, Ts...>)
  constexpr static object_ref add_object(Ts &&... ts) {
    objects.emplace_back(std::forward<Ts>(ts)...);
    return object_ref{objects.size() - 1};
  }

  /**
   * @brief Load a shader from assets, and add it to the cache.
   * @param vs The path to the vertex shader asset.
   * @param fs The path to the fragment shader asset.
   * @return A reference to the shader.
   *
   * The shader is loaded using the @ref `shader::from_assets` function, and then added to the cache.
   */
  constexpr static shader_ref load_shader(const std::string &vs, const std::string &fs) {
    shaders.emplace_back(shader::from_assets(vs, fs));
    return shader_ref{shaders.size() - 1};
  }

  /**
   * @brief Add a shader to the cache.
   * @tparam Ts The types of the arguments to the shader's constructor.
   * @param ts The arguments to the shader's constructor.
   * @return A reference to the shader.
   *
   * The shader is constructed in-place in the cache.
   */
  template <typename ... Ts> requires(std::constructible_from<shader, Ts...>)
  constexpr static shader_ref add_shader(Ts &&... ts) {
    shaders.emplace_back(std::forward<Ts>(ts)...);
    return shader_ref{shaders.size() - 1};
  }

  /**
   * @brief Add a texture to the cache.
   * @tparam Ts The types of the arguments to the texture's constructor.
   * @param ts The arguments to the texture's constructor.
   * @return A reference to the texture.
   *
   * The texture is constructed in-place in the cache.
   */
  template <typename ... Ts> requires(std::constructible_from<texture, Ts...>)
  constexpr static texture_ref add_texture(Ts &&... ts) {
    textures.emplace_back(std::forward<Ts>(ts)...);
    return texture_ref{textures.size() - 1};
  }

  /**
   * @brief Add a renderable to the cache.
   * @tparam Ts The types of the arguments to the renderable's constructor.
   * @param ts The arguments to the renderable's constructor.
   * @return A reference to the renderable.
   *
   * The renderable is constructed in-place in the cache.
   */
  template <typename ... Ts> requires(std::constructible_from<renderable, Ts...>)
  constexpr static render_ref add_renderable(Ts &&... ts) {
    renderables.emplace_back(std::forward<Ts>(ts)...);
    return render_ref{renderables.size() - 1};
  }

  /**
   * @brief Render an overview of the cache contents.
   *
   * The overview includes the amount of objects, shaders, and textures, as well as a more detailed view of the
   * renderables (with their position, rotation, and scale). Renderables can be enabled and disabled in the UI.
   */
  static void detail_window();

private:
  static inline std::vector<render_object> objects{}; //!< The list of objects in the cache.
  static inline std::vector<shader> shaders{}; //!< The list of shaders in the cache.
  static inline std::vector<texture> textures{}; //!< The list of textures in the cache.
  static inline std::vector<renderable> renderables{}; //!< The list of renderables in the cache.
};

/**
 * @brief A global instance of the cache for ease-of-use.
 */
constexpr static inline auto cache = render_cache{};

/**
 * @brief Gets the object the reference points to.
 * @param r The object reference.
 * @return The object the reference points to.
 *
 * This overload allows the object reference to be treated as a pointer.
 */
constexpr render_object &operator*(const object_ref &r) { return cache[r]; }
/**
 * @brief Gets the shader the reference points to.
 * @param r The shader reference.
 * @return The shader the reference points to.
 *
 * This overload allows the shader reference to be treated as a pointer.
 */
constexpr shader &operator*(const shader_ref &r) { return cache[r]; }
/**
 * @brief Gets the texture the reference points to.
 * @param r The texture reference.
 * @return The texture the reference points to.
 *
 * This overload allows the texture reference to be treated as a pointer.
 */
constexpr texture &operator*(const texture_ref &r) { return cache[r]; }
/**
 * @brief Gets the renderable the reference points to.
 * @param r The renderable reference.
 * @return The renderable the reference points to.
 *
 * This overload allows the renderable reference to be treated as a pointer.
 */
constexpr renderable &operator*(const render_ref &r) { return cache[r]; }
}

#endif //RENDER_CACHE_HPP
