//
// Created by jay on 11/30/24.
//

#ifndef RENDER_CACHE_HPP
#define RENDER_CACHE_HPP

#include <vector>

#include "util.hpp"
#include "camera.hpp"
#include "window.hpp"
#include "object.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "collider.hpp"

namespace openvtt::renderer {
/**
 * @brief A class representing a reference to an object in the cache.
 * @tparam T The type of object to reference.
 */
template <typename T>
class t_ref {
public:
  /**
   * @brief Derefences the reference.
   * @return The object the reference points to.
   */
  constexpr T *operator->() const { return &**this; }

  /**
   * @brief Compares two references for equality.
   * @param other The reference to compare to.
   * @return `true` if the references point to the same object, `false` otherwise.
   */
  constexpr bool operator==(const t_ref &other) const { return idx == other.idx; }
  /**
   * @brief Compares two references for inequality.
   * @param other The reference to compare to.
   * @return `true` if the references point to different objects, `false` otherwise.
   */
  constexpr bool operator!=(const t_ref &other) const { return idx != other.idx; }
  /**
   * @brief Constructs a new, invalid reference.
   * @return The invalid reference.
   */
  static constexpr t_ref invalid() { return t_ref{-1UL}; }
  /**
   * @brief Gets the raw value of the reference.
   * @return The raw value of the reference.
   */
  [[nodiscard]] constexpr size_t raw() const { return idx; }

private:
  constexpr explicit t_ref(const size_t idx) : idx{idx} {}
  size_t idx;
  friend class render_cache;
};

template <typename T>
constexpr T &operator*(const t_ref<T> &r);

using object_ref = t_ref<render_object>; //!< Type alias for a reference to a render object.
using instanced_object_ref = t_ref<instanced_object>; //!< Type alias for a reference to an instanced render object.
using voxel_ref = t_ref<voxel_group>; //!< Type alias for a reference to a voxel group.
using shader_ref = t_ref<shader>; //!< Type alias for a reference to a shader.
using texture_ref = t_ref<texture>; //!< Type alias for a reference to a texture.
using collider_ref = t_ref<collider>; //!< Type alias for a reference to a collider.
using instanced_collider_ref = t_ref<instanced_collider>; //!< Type alias for a reference to an instanced collider.

class render_cache;
}

/**
 * @brief Template specialization for hashing a reference.
 * @tparam T The type of object to reference.
 */
template <typename T>
struct std::hash<openvtt::renderer::t_ref<T>> { // NOLINT(*-dcl58-cpp) // false positive
  static size_t operator()(const openvtt::renderer::t_ref<T> &ref) noexcept {
    return std::hash<size_t>{}(ref.raw());
  }
};

/**
 * @brief Template specialization for formatting a reference.
 * @tparam T The type of object to reference.
 */
template <typename T>
struct std::formatter<openvtt::renderer::t_ref<T>, char> { // NOLINT(*-dcl58-cpp) // false positive
  using type = openvtt::renderer::t_ref<T>;
  std::formatter<std::string> base{};
  template <typename PC> constexpr auto parse(PC &ctx) { return base.parse(ctx); }
  template <typename FC> constexpr auto format(const type &ref, FC &ctx) { return base.format(ref.desc(), ctx); }
};

#include "renderable.hpp"

namespace openvtt::renderer {
using render_ref = t_ref<renderable>; //!< Type alias for a reference to a renderable.
using instanced_render_ref = t_ref<instanced_renderable>; //!< Type alias for a reference to an instanced renderable.

/**
 * @brief Namespace for type traits.
 */
namespace type_traits {
/**
 * @brief Concept for checking if two types are the same, ignoring cv-qualifiers and references.
 */
template <typename T1, typename T2>
concept cvr_same = std::same_as<std::remove_cvref_t<T1>, std::remove_cvref_t<T2>>;
/**
 * @brief A concept that is always false.
 */
template <typename T>
concept invalid = false;

/**
 * @brief Concept for checking if a type is loadable from a set of arguments.
 *
 * For this, the type must have a static member function `load_from` that returns the type itself, and takes the
 * arguments provided.
 */
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
  /**
   * @brief Construct a new render cache.
   */
  constexpr render_cache() = default;

  /**
   * @brief Dereferences a reference to a value in the cache.
   * @tparam T The type of object to dereference.
   * @param ref The reference (`t_ref<T>`) to the object.
   * @return A reference (`T &`) to the object in the cache.
   */
  template <typename T>
  constexpr static T &operator[](const t_ref<T> &ref) {
    return cache_for<T>()[ref.idx];
  }

  /**
   * @brief Constructs a new value in the cache.
   * @tparam T The type of object to construct.
   * @tparam Args The types of the arguments to pass to the constructor.
   * @param args The arguments to pass to the constructor.
   * @return A reference to the newly constructed object.
   */
  template <typename T, typename ... Args> requires(std::constructible_from<T, Args...>)
  constexpr static t_ref<T> construct(Args &&... args) {
    cache_for<T>().emplace_back(std::forward<Args>(args)...);
    return last_for<T>();
  }

  /**
   * @brief Loads a value into the cache.
   * @tparam T The type of object to load (should satisfy the `loadable` concept).
   * @tparam Args The types of the arguments to pass to the `load_from` function.
   * @param args The arguments to pass to the `load_from` function.
   * @return A reference to the loaded object.
   */
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
   * @brief Gets the mouse position in world coordinates, projected onto the XZ plane.
   * @return The mouse position in world coordinates.
   */
  static glm::vec2 mouse_y0(const camera &cam);

  /**
   * @brief Placeholder type indicating no collision.
   */
  struct no_collision{};

  /**
   * @brief The result of a collision check.
   *
   * This is one of:
   * - `no_collision`: No collision occurred.
   * - `render_ref`: A collision occurred with a renderable.
   * - `std::pair<instanced_render_ref, size_t>`: A collision occurred with an instanced renderable, and the index of the
   * instance is returned as well.
   */
  using collision_res = std::variant<no_collision, render_ref, std::pair<instanced_render_ref, size_t>>;

  /**
   * @brief Checks if the mouse is hovering over any collider.
   * @param cam The camera to generate the ray with.
   * @return The reference to the collider the mouse is hovering over, if any.
   *
   * This is a very expensive function to call (O(n) in the number of objects), and should be used at most once per
   * frame. Ray collision detection is optimized by using AABB's in the collider.
   */
  static collision_res mouse_over(const camera &cam);

  /**
   * @brief Executes a function depending on what kind of object the mouse is hovering over.
   * @tparam F1 The function to execute if the mouse is hovering over a (single) renderable (signature `(render_ref) -> void`).
   * @tparam F2 The function to execute if the mouse is hovering over an instanced renderable (signature `(instanced_render_ref, size_t) -> void`).
   * @param cam The camera to generate the ray with.
   * @param on_single The function to execute if the mouse is hovering over a single renderable.
   * @param on_instanced The function to execute if the mouse is hovering over an instanced renderable.
   *
  * * This is a very expensive function to call (O(n) in the number of objects), and should be used at most once per
   * frame. Ray collision detection is optimized by using AABB's in the collider.
   *
   * The `on_instanced` function is passed both a reference to the instanced renderable and the index of the specific
   * instance the mouse is hovering over.
   */
  template <std::invocable<render_ref> F1, std::invocable<instanced_render_ref, size_t> F2>
  static void with_mouse_over(const camera &cam, F1 &&on_single, F2 &&on_instanced) {
    if (const auto hover = mouse_over(cam); std::holds_alternative<render_ref>(hover)) {
      on_single(std::get<render_ref>(hover));
    }
    else if (std::holds_alternative<std::pair<instanced_render_ref, size_t>>(hover)) {
      auto &[ref, idx] = std::get<std::pair<instanced_render_ref, size_t>>(hover);
      on_instanced(ref, idx);
    }
  }

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
    else if constexpr(type_traits::cvr_same<T, instanced_collider>) return instanced_colliders;
    else if constexpr(type_traits::cvr_same<T, voxel_group>) return voxels;
    else {
      static_assert(type_traits::invalid<T>, "Type not supported in cache.");
      OPENVTT_UNREACHABLE;
    }
  }

  template <typename T>
  static constexpr t_ref<T> last_for() {
    return t_ref<T>{cache_for<T>().size() - 1};
  }

  static inline std::optional<shader_ref> collider_shader{}; //!< The shader to render the colliders with, if any.
  static inline std::optional<shader_ref> collider_instanced_shader{}; //!< The instanced shader to render the colliders with, if any.
  static inline std::vector<render_object> objects{}; //!< The list of objects in the cache.
  static inline std::vector<instanced_object> instanced_objects{}; //!< The list of instanced objects in the cache.
  static inline std::vector<voxel_group> voxels{}; //!< The list of voxel groups in the cache.
  static inline std::vector<shader> shaders{}; //!< The list of shaders in the cache.
  static inline std::vector<texture> textures{}; //!< The list of textures in the cache.
  static inline std::vector<renderable> renderables{}; //!< The list of renderables in the cache.
  static inline std::vector<instanced_renderable> instanced_renderables{}; //!< The list of instanced renderables in the cache.
  static inline std::vector<collider> colliders{}; //!< The list of colliders in the cache.
  static inline std::vector<instanced_collider> instanced_colliders{}; //!< The list of instanced colliders in the cache.
  static inline bool render_colliders = false; //!< Whether to render the colliders.
};

/**
 * @brief A global instance of the cache for ease-of-use.
 */
constexpr static inline auto cache = render_cache{};

/**
 * @brief Dereferences a reference to a value in the cache.
 * @tparam T The type of object to dereference.
 * @param r The reference to the object.
 * @return The object the reference points to.
 */
template <typename T>
constexpr T &operator*(const t_ref<T> &r) { return cache[r]; }
}

#endif //RENDER_CACHE_HPP
