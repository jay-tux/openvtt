//
// Created by jay on 11/30/24.
//

#ifndef RENDER_CACHE_HPP
#define RENDER_CACHE_HPP

#include <vector>

#include "camera.hpp"
#include "window.hpp"
#include "object.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "collider.hpp"

namespace openvtt::renderer {
template <typename T>
class t_ref {
public:
  constexpr T *operator->() const { return &**this; }

private:
  constexpr explicit t_ref(const size_t idx) : idx{idx} {}
  size_t idx;
  friend class render_cache;
};

template <typename T>
constexpr T &operator*(const t_ref<T> &r);

using object_ref = t_ref<render_object>;
using instanced_object_ref = t_ref<instanced_object>;
using shader_ref = t_ref<shader>;
using texture_ref = t_ref<texture>;
using collider_ref = t_ref<collider>;
class render_cache;
}

#include "renderable.hpp"

namespace openvtt::renderer {
using render_ref = t_ref<renderable>;
using instanced_render_ref = t_ref<instanced_renderable>;

namespace type_traits {
template <typename T1, typename T2>
concept cvr_same = std::same_as<std::remove_cvref_t<T1>, std::remove_cvref_t<T2>>;
template <typename T>
concept invalid = false;

template <typename T, typename ... Args>
concept loadable = requires(Args &&... args)
{
  { T::load_from(std::forward<Args>(args)...) } -> std::same_as<T>;
};
}

/**
 * @brief A cache of render objects, shaders, textures, colliders, and renderables.
 *
 * Each of the types is stored contiguously in memory, and can be accessed using a reference to the cache. Pointers and
 * references to any object, shader, texture, or renderable can be invalidated if the cache is modified, so it is
 * recommended to not store them. Use the references from the cache instead.
 */
class render_cache {
public:
  constexpr render_cache() = default;

  template <typename T>
  constexpr static T &operator[](const t_ref<T> &ref) {
    return cache_for<T>()[ref.idx];
  }

  template <typename T, typename ... Args> requires(std::constructible_from<T, Args...>)
  constexpr static t_ref<T> construct(Args &&... args) {
    cache_for<T>().emplace_back(std::forward<Args>(args)...);
    return last_for<T>();
  }

  template <typename T, typename ... Args> requires(type_traits::loadable<T, Args...>)
  constexpr static t_ref<T> load(Args &&... args) {
    cache_for<T>().emplace_back(T::load_from(std::forward<Args>(args)...));
    return last_for<T>();
  }

  /**
   * @brief Duplicate a renderable in the cache.
   * @param ref The reference to the renderable to duplicate.
   * @param name The name of the new renderable.
   * @param pos The position of the new renderable; if not provided, the position of the original renderable is used.
   * @param rot The rotation of the new renderable; if not provided, the rotation of the original renderable is used.
   * @param scale The scale of the new renderable; if not provided, the scale of the original renderable is used.
   * @return A reference to the new renderable.
   *
   * While the renderable itself is copied, it will still use the same object, shader, and texture as the original.
   */
  static render_ref duplicate(
    const render_ref &ref, const std::string &name, const std::optional<glm::vec3> &pos = std::nullopt,
    const std::optional<glm::vec3> &rot = std::nullopt, const std::optional<glm::vec3> &scale = std::nullopt
  );

  /**
   * @brief Render an overview of the cache contents.
   *
   * The overview includes the amount of objects, shaders, and textures, as well as a more detailed view of the
   * renderables (with their position, rotation, and scale). Renderables can be enabled and disabled in the UI.
   */
  static void detail_window();

  /**
   * @brief Render the colliders for all active renderables.
   * @param cam The camera to render the colliders with.
   *
   * If an active renderable does not have a collider associated with it, it is ignored.
   *
   * This will initialize the `collider_shader` field, if it is not already initialized. For this, it relies on the
   * `basic_mvp` vertex shader (`/assets/shaders/basic_mvp.vs.glsl`), and the `collider` fragment shader
   * (`/assets/shaders/collider.fs.glsl`).
   */
  static void draw_colliders(const camera &cam);

  /**
   * @brief Checks if the mouse is hovering over any collider.
   * @param cam The camera to generate the ray with.
   * @return The reference to the collider the mouse is hovering over, if any.
   *
   * This is a very expensive function to call (O(n) in the number of objects), and should be used at most once per
   * frame. Ray collision detection is optimized by using AABB's in the collider.
   */
  static std::optional<render_ref> mouse_over(const camera &cam);

  /**
   * @brief Checks whether we should render the colliders.
   * @return Whether we should render the colliders.
   */
  static constexpr bool should_render_colliders() { return render_colliders; }

private:
  template <typename T>
  static constexpr std::vector<T> &cache_for() {
    if constexpr(type_traits::cvr_same<T, render_object>) return objects;
    else if constexpr(type_traits::cvr_same<T, instanced_object>) return instanced_objects;
    else if constexpr(type_traits::cvr_same<T, shader>) return shaders;
    else if constexpr(type_traits::cvr_same<T, texture>) return textures;
    else if constexpr(type_traits::cvr_same<T, renderable>) return renderables;
    else if constexpr(type_traits::cvr_same<T, instanced_renderable>) return instanced_renderables;
    else if constexpr(type_traits::cvr_same<T, collider>) return colliders;
    else {
      static_assert(type_traits::invalid<T>, "Type not supported in cache.");
      std::unreachable();
    }
  }

  template <typename T>
  static constexpr t_ref<T> last_for() {
    return t_ref<T>{cache_for<T>().size() - 1};
  }

  static inline std::optional<shader_ref> collider_shader{}; //!< The shader to render the colliders with, if any.
  static inline std::vector<render_object> objects{}; //!< The list of objects in the cache.
  static inline std::vector<instanced_object> instanced_objects{}; //!< The list of instanced objects in the cache.
  static inline std::vector<shader> shaders{}; //!< The list of shaders in the cache.
  static inline std::vector<texture> textures{}; //!< The list of textures in the cache.
  static inline std::vector<renderable> renderables{}; //!< The list of renderables in the cache.
  static inline std::vector<instanced_renderable> instanced_renderables{}; //!< The list of instanced renderables in the cache.
  static inline std::vector<collider> colliders{}; //!< The list of colliders in the cache.
  static inline bool render_colliders = false; //!< Whether to render the colliders.
};

/**
 * @brief A global instance of the cache for ease-of-use.
 */
constexpr static inline auto cache = render_cache{};

template <typename T>
constexpr T &operator*(const t_ref<T> &r) { return cache[r]; }
}

#endif //RENDER_CACHE_HPP
