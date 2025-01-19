//
// Created by jay on 1/1/25.
//

#ifndef MAP_BUILTINS_HPP
#define MAP_BUILTINS_HPP

#include "either.hpp"
#include "object_cache.hpp"
#include "map_visitor.hpp"
#include "renderer/log_view.hpp"
#include "scanline.hpp"

/**
 * @brief Namespace for map parsing.
 */
namespace openvtt::map {
/**
 * @brief Type alias for either a value or an error.
 */
template <typename T> using or_error = either<std::string, T>;

/**
 * @brief Checks if a function is called with the correct number of arguments.
 * @param args The provided arguments.
 * @param n The expected number of arguments.
 * @param function The function name.
 * @param pos The positions of the call
 * @return Either the arguments (again) or an error message.
 */
inline or_error<std::vector<value>> n_args(const std::vector<value> &args, const size_t n, const std::string &function, const loc &pos) {
  if (args.size() != n) return left(std::format("Function {} expects {} arguments, but got {} at {}.", function, n, args.size(), pos.str()));
  return right(args);
}

/**
 * @brief Type-checks a value.
 * @tparam T The expected type.
 * @param v The value to check.
 * @return The contained value, or and error message.
 *
 * If the provided value is of the required type, returns that value as a `right(value)`.
 * Otherwise, an error message is returned as a `left(error)`.
 */
template <valid_value T>
inline or_error<T> type_check(const value &v) {
  if (v.is<T>()) return right(v.as<T>());
  if constexpr(std::same_as<T, float>) {
    if (v.is<int>()) return right(static_cast<float>(v.as<int>()));
  }
  return left(std::format("Expected a value of type {}, but got {} at {}.", type_name<T>(), v.type_name(), v.pos().str()));
}

/**
 * @brief Type-checks a set of values.
 * @tparam Ts The expected types.
 * @param vs The values to check.
 * @return A tuple of the contained values, or an error message.
 *
 * If any of the values are not of the required type, an error message indicating the first mismatch is returned.
 * Otherwise, the values are returned as a tuple, in the same order as the provided values.
 */
template <valid_value ... Ts>
or_error<std::tuple<Ts...>> type_check_multi(const std::array<value, sizeof...(Ts)> &vs) {
  using tup = std::tuple<Ts...>;
  constexpr auto lambda = []<size_t ... idxs>(const std::array<value, sizeof...(Ts)> &v, std::index_sequence<idxs...>) {
    return merge(type_check<std::tuple_element_t<idxs, tup>>(v[idxs])...);
  };

  return lambda(vs, std::make_index_sequence<sizeof...(Ts)>{});
}

/**
 * @brief Check if all values in the vector have the same type.
 * @tparam T The expected type.
 * @param vs The values to check.
 * @return A vector with the contained values, or an error message.
 *
 * If any of the values in the vector is not of the required type, an error message indicating the first mismatch is
 * returned.
 * Otherwise, all values are "unwrapped" and returned as a new vector.
 */
template <typename T>
inline or_error<std::vector<T>> type_check_vector(const std::vector<value> &vs) {
  std::vector<T> res;
  res.reserve(vs.size());
  for (const auto &v: vs) {
    if (const auto x = type_check<T>(v); x.is_right()) res.push_back(x.right());
    else return left(x.left());
  }
  return right(res);
}

/**
 * @brief Type-checks a vector of values.
 * @tparam F A helper type-check function (should be of type (const value_pair &) -> or_error<T>).
 * @param vs The values to check.
 * @param f The function.
 * @return Either a vector of the contained values, or an error message.
 *
 * First, we ensure that all values in the vector are of the type `value_pair`.
 * If any of them are not, an error message indicating the first mismatch is returned.
 * For each valid `value_pair`, `f` is invoked with the pair as argument.
 * If `f` returns an error, that error is returned.
 *
 * We loop once over the entire list, meaning that each value is checked to be a `value_pair` and then immediately after,
 * `f` is invoked.
 * You cannot assume that all values in the list are `value_pair`s upon the first invocation of `f`.
 */
template <std::invocable<const value_pair &> F>
requires(is_either<res_t<F, const value_pair &>>)
inline or_error<std::vector<typename either_traits<res_t<F, const value_pair &>>::right_t>> type_check_pair_vector(const std::vector<value> &vs, F &&f) {
  using mapped_t = typename either_traits<res_t<F, const value_pair &>>::right_t;
  std::vector<mapped_t> res;
  res.reserve(vs.size());
  for (const auto &v: vs) {
    if (const auto x = type_check<value_pair>(v); x.is_right()) {
      if (const auto y = f(x.right()); y.is_right()) res.push_back(y.right());
      else return left(y.left());
    }
    else return left(x.left());
  }
  return right(res);
}

/**
 * @brief Checks if a function is called with exactly one argument of the provided type.
 * @tparam T The expected argument type.
 * @param args The provided arguments.
 * @param function The function name.
 * @param pos The position of the call.
 * @return Either the first (and only) value in the list, or an error message.
 *
 * This is a convenience function wrapping `n_args` (for 1 argument) and `type_check<T>`.
 */
template <valid_value T>
or_error<T> ready_arg(const std::vector<value> &args, const std::string &function, const loc &pos) {
  return n_args(args, 1, function, pos) >> [](const std::vector<value> &a) { return type_check<T>(a[0]); };
}

/**
 * @brief Checks if a function is called with the exact amount of arguments of the provided types.
 * @tparam Ts The expected argument types.
 * @param args The provided arguments.
 * @param function The function name.
 * @param pos The position of the call.
 * @return Either a tuple of the contained values, or an error message.
 *
 * This is a convenience function wrapping `n_args` (for `sizeof...(Ts)` arguments) and `type_check_multi<Ts...>`.
 */
template <valid_value ... Ts>
or_error<std::tuple<Ts...>> ready_args(const std::vector<value> &args, const std::string &function, const loc &pos) {
  constexpr static auto lambda = []<size_t ... idxs>(const std::vector<value> &a, std::index_sequence<idxs...>) {
    return type_check_multi<Ts...>({a[idxs]...});
  };

  return n_args(args, sizeof...(Ts), function, pos)
    >> [](const std::vector<value> &a) { return lambda(a, std::make_index_sequence<sizeof...(Ts)>{}); };
}

/**
 * @brief Handles the result of a function call.
 * @tparam T The provided value.
 * @param res The result to be unwrapped.
 * @param pos The position of the call.
 * @param backup The value to be returned in case of errors.
 * @return Either the contained value, or the backup value.
 *
 * If the provided value (which is expected to be the result of invoking a builtin function) is an error, the error is
 * logged, and the default (backup) value is returned instead.
 * Otherwise, the (successful) return value of the function is passed along.
 */
template <valid_value T>
value handle(const or_error<T> &res, const loc &pos, const T &backup = {}) {
  return fold(
    map_left(res, [&backup](const std::string &err){ renderer::log<renderer::log_type::WARNING>("map_loader", err); return backup; }),
    [&pos](const T &x) -> value { return value{x, pos}; },
    [&pos](const T &x) -> value { return value{x, pos}; }
  );
}

/**
 * @brief Handles the result of a function call that returns no value.
 * @tparam T The provided value (ignored).
 * @param res The result to be unwrapped.
 * @param pos The position of the call.
 * @return A value wrapping `std::monostate` (void).
 *
 * If the provided value is an error, the error is logged.
 * After the check, this function returns a value of type `std::monostate` (which is used to indicate "no value").
 */
template <typename T>
value handle_no_value(const or_error<T> &res, const loc &pos) {
  map_left(res, [](const std::string &err) { renderer::log<renderer::log_type::WARNING>("map_loader", err); return std::monostate{}; });
  return value{std::monostate{}, pos};
}

/**
 * @brief Verifies if a builtin function running in the given visitor is executed with the correct scope.
 * @tparam s The expected scope.
 * @param func The function name.
 * @param v The map visitor.
 * @param pos The position of the call.
 * @return Either an empty success value (`either_tag`), or an error message.
 */
template <map_visitor::scope s>
inline or_error<either_tag> requires_scope(const std::string &func, const map_visitor &v, const loc &pos) {
  constexpr static auto scope_name = [](const map_visitor::scope &scope) -> std::string {
    switch (scope) {
      case map_visitor::scope::NONE: return "(no scope)";
      case map_visitor::scope::VOXEL: return "a voxel scope";
      case map_visitor::scope::OBJECTS: return "an objects scope";
    }
    OPENVTT_UNREACHABLE;
  };

  if (v.current_scope != s) return left(std::format("Function {} requires {} (at {})", func, scope_name(s), pos.str()));
  return right(either_tag{});
}

/**
 * @brief Invokes the builtin `object` function.
 * @param args The arguments from the parser.
 * @param v The map visitor.
 * @param pos The position of the call.
 * @return Either a reference to the (loaded) object, or an invalid reference.
 *
 * The `object` builtin loads an object (mesh) from an asset file.
 * This function expects a single `string` argument.
 */
inline value invoke_object(const std::vector<value> &args, map_visitor &v, const loc &pos) {
  return handle(
    requires_scope<map_visitor::scope::OBJECTS>("@object", v, pos) >>
    [&args, &pos] { return ready_arg<std::string>(args, "@object", pos); } |
    [](const std::string &asset) { return renderer::render_cache::load<renderer::render_object>(asset); },

    pos, renderer::object_ref::invalid());
}

/**
 * @brief Invokes the builtin `object*` function.
 * @param args The arguments from the parser.
 * @param v The map visitor.
 * @param pos The position of the call.
 * @return Either a reference to the (loaded) instanced object, or an invalid reference.
 *
 * The `object*` builtin loads an object (mesh) from an asset file, and creates a set of instances from it.
 * This function expects a `string` argument (the asset file) and a vector of `mat4` values (the transforms).
 */
inline value invoke_object_star(const std::vector<value> &args, map_visitor &v, const loc &pos) {
  return handle(
  requires_scope<map_visitor::scope::OBJECTS>("@object*", v, pos) >>
    [&args, &pos] { return ready_args<std::string, std::vector<value>>(args, "@object*", pos); } >>
    [](const std::tuple<std::string, std::vector<value>> &tup) {
      const auto &[asset, transforms] = tup;
      return type_check_vector<glm::mat4>(transforms) | [&asset](const auto &mats) { return std::pair{asset, mats}; };
    } |
    [](const std::pair<std::string, std::vector<glm::mat4>> &p) {
      const auto &[asset, transforms] = p;
      return renderer::render_cache::load<renderer::instanced_object>(asset, transforms);
    },

    pos, renderer::instanced_object_ref::invalid()
  );
}

/**
 * @brief Invokes the builtin `shader` function.
 * @param args The arguments from the parser.
 * @param v The map visitor.
 * @param pos The position of the call.
 * @return Either a reference to the (loaded) shader, or an invalid reference.
 *
 * The `shader` builtin loads a shader pair (vertex and fragment) from the respective asset files.
 * This function expects two `string` arguments.
 */
inline value invoke_shader(const std::vector<value> &args, map_visitor &v, const loc &pos) {
  return handle(
  requires_scope<map_visitor::scope::OBJECTS>("@shader", v, pos) >>
    [&args, &pos] { return ready_args<std::string, std::string>(args, "@shader", pos); } |
    [](const std::tuple<std::string, std::string> &vf) { return renderer::render_cache::load<renderer::shader>(std::get<0>(vf), std::get<1>(vf)); },

    pos, renderer::shader_ref::invalid()
  );
}

/**
 * @brief Invokes the builtin `texture` function.
 * @param args The arguments from the parser.
 * @param v The map visitor.
 * @param pos The position of the call.
 * @return Either a reference to the (loaded) texture, or an invalid reference.
 *
 * The `texture` builtin loads texture from an asset file.
 * This function expects a single `string` argument (the asset file).
 */
inline value invoke_texture(const std::vector<value> &args, map_visitor &v, const loc &pos) {
  return handle(
  requires_scope<map_visitor::scope::OBJECTS>("@texture", v, pos) >>
    [&args, &pos] { return ready_arg<std::string>(args, "@texture", pos); } |
    [](const std::string &asset) { return renderer::render_cache::construct<renderer::texture>(asset); },

    pos, renderer::texture_ref::invalid()
  );
}

/**
 * @brief Invokes the builtin `collider` function.
 * @param args The arguments from the parser.
 * @param v The map visitor.
 * @param pos The position of the call.
 * @return Either a reference to the (loaded) collider, or an invalid reference.
 *
 * The `collider` builtin loads a collider (mesh) from an asset file.
 * This function expects a single `string` argument (the asset file).
 */
inline value invoke_collider(const std::vector<value> &args, map_visitor &v, const loc &pos) {
  return handle(
  requires_scope<map_visitor::scope::OBJECTS>("@collider", v, pos) >>
    [&args, &pos] { return ready_arg<std::string>(args, "@collider", pos); } |
    [](const std::string &asset) { return renderer::render_cache::load<renderer::collider>(asset); },

    pos, renderer::collider_ref::invalid()
  );
}

/**
 * @brief Invokes the builtin `collider*` function.
 * @param args The arguments from the parser.
 * @param v The map visitor.
 * @param pos The position of the call.
 * @return Either a reference to the (loaded) instanced collider, or an invalid reference.
 *
 * The `collider*` builtin loads a collider (mesh) from an asset file, and creates a set of instances from it.
 * This function expects a `string` argument (the asset file) and a vector of `mat4` values (the transforms).
 */
inline value invoke_collider_star(const std::vector<value> &args, map_visitor &v, const loc &pos) {
  return handle(
  requires_scope<map_visitor::scope::OBJECTS>("@collider*", v, pos) >>
    [&args, &pos] { return ready_args<std::string, std::vector<value>>(args, "@collider*", pos); } >>
    [](const std::tuple<std::string, std::vector<value>> &tup) {
      const auto &[asset, transforms] = tup;
      return type_check_vector<glm::mat4>(transforms) | [&asset](const auto &mats) { return std::pair{asset, mats}; };
    } |
    [](const std::pair<std::string, std::vector<glm::mat4>> &p) {
      const auto &[asset, transforms] = p;
      return renderer::render_cache::load<renderer::instanced_collider>(asset, transforms);
    },

    pos, renderer::instanced_collider_ref::invalid()
  );
}

/**
 * @brief Invokes the builtin `transform` function.
 * @param args The arguments from the parser.
 * @param v The map visitor.
 * @param pos The position of the call.
 * @return Either a model matrix, or the identity matrix.
 *
 * The `transform` builtin constructs a model matrix from the provided position, rotation, and scale vectors.
 * This function expects three `vec3` arguments.
 */
inline value invoke_transform(const std::vector<value> &args, map_visitor &v, const loc &pos) {
  return handle(
  requires_scope<map_visitor::scope::OBJECTS>("@transform", v, pos) >>
    [&args, &pos] { return ready_args<glm::vec3, glm::vec3, glm::vec3>(args, "@transform", pos); } |
    [](const std::tuple<glm::vec3, glm::vec3, glm::vec3> &tup) {
      const auto &[pos, rot, scale] = tup;
      return renderer::instanced_object::model_for(rot, scale, pos);
    },

    pos, glm::mat4(1.0f)
  );
}

/**
 * @brief Invokes the builtin `spawn` function.
 * @param args The arguments from the parser.
 * @param cache The map visitor cache.
 * @param pos The position of the call.
 * @return Either a reference to the spawned object, or an invalid reference.
 *
 * The `spawn` builtin creates a new renderable object from the provided object, shader, and textures list.
 * This function expects a `string` (name), an `object` reference, a `shader` reference, and a vector of
 * `(int, texture)` pairs (indicating which textures are to be bound to which uniforms in the shader).
 */
inline value invoke_spawn(const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  return handle(
  requires_scope<map_visitor::scope::OBJECTS>("@spawn", cache, pos) >>
    // check arguments
    [&args, &pos] { return ready_args<std::string, renderer::object_ref, renderer::shader_ref, std::vector<value>>(args, "@spawn", pos); } >>
    [&cache](const auto &a) -> or_error<renderer::render_ref> {
      // check textures
      const auto &[name, obj, sh, textures] = a;

      return type_check_pair_vector(textures, [](const value_pair &vp) {
        return type_check_multi<int, renderer::texture_ref>({vp.first(), vp.second()}) |
          [](const std::pair<int, renderer::texture_ref> &p) { return std::pair{static_cast<unsigned int>(p.first), p.second}; };
      }) |
      // insert into render cache
      [&name, &obj, &sh](const std::vector<std::pair<unsigned int, renderer::texture_ref>> &tex) {
        return renderer::render_cache::construct<renderer::renderable>(name, obj, sh, renderer::uniforms::from_shader(sh), tex);
      } |
      // insert into spawn cache
      [&cache](const renderer::render_ref &rr) { cache.spawned.insert(rr); return rr; };
    },

    pos, renderer::render_ref::invalid()
  );
}

/**
 * @brief Invokes the builtin `spawn*` function.
 * @param args The arguments from the parser.
 * @param cache The map visitor cache.
 * @param pos The position of the call.
 * @return Either a reference to the spawned instanced object, or an invalid reference.
 *
 * The `spawn*` builtin creates a new instanced renderable object from the provided instanced object, shader, and
 * textures list.
 * This function expects a `string` (name), an `object*` reference, a `shader` reference, and a vector of
 * `(int, texture)` pairs (indicating which textures are to be bound to which uniforms in the shader).
 */
inline value invoke_spawn_star(const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  return handle(
  requires_scope<map_visitor::scope::OBJECTS>("@spawn*", cache, pos) >>
    // check arguments
    [&args, &pos] { return ready_args<std::string, renderer::instanced_object_ref, renderer::shader_ref, std::vector<value>>(args, "@spawn*", pos); } >>
    [](const auto &a) -> or_error<renderer::instanced_render_ref> {
      // check textures
      const auto &[asset, obj, sh, textures] = a;
      return type_check_pair_vector(textures, [](const value_pair &vp) {
        return type_check_multi<int, renderer::texture_ref>({vp.first(), vp.second()}) |
          [](const std::pair<int, renderer::texture_ref> &p) { return std::pair{static_cast<unsigned int>(p.first), p.second}; };
      }) |
      // insert into render cache
      [&asset, &obj, &sh](const std::vector<std::pair<unsigned int, renderer::texture_ref>> &tex) {
        return renderer::render_cache::construct<renderer::instanced_renderable>(asset, obj, sh, renderer::instanced_uniforms::from_shader(sh), tex);
      };
    } |
    // insert into spawn cache
    [&cache](const renderer::instanced_render_ref &rr) { cache.spawned_instances.insert(rr); return rr; },

    pos, renderer::instanced_render_ref::invalid()
  );
}

/**
 * @brief Invokes the builtin `transform_obj` function.
 * @param args The arguments from the parser.
 * @param v The map visitor.
 * @param pos The position of the call.
 * @return A value wrapping `std::monostate`.
 *
 * The `transform_obj` builtin sets the position, rotation, and scale of the provided renderable object.
 * This function expects a `renderable` reference, and three `vec3` arguments (position, rotation, and scale).
 */
inline value invoke_transform_obj(const std::vector<value> &args, map_visitor &v, const loc &pos) {
  return handle_no_value(
  requires_scope<map_visitor::scope::OBJECTS>("@transform_obj", v, pos) >>
    [&args, &pos] { return ready_args<renderer::render_ref, glm::vec3, glm::vec3, glm::vec3>(args, "@transform_obj", pos); } |
    []<typename ... Ts>(const std::tuple<Ts...> &tup) {
      const auto &[rr, p, r, s] = tup;
      rr->position = p; rr->rotation = r; rr->scale = s;
      return std::monostate{};
    },

    pos
  );
}

/**
 * @brief Invokes the builtin `enable_highlight` function.
 * @param args The arguments from the parser.
 * @param cache The map visitor cache.
 * @param pos The position of the call.
 * @return A value wrapping `std::monostate`.
 *
 * The `enable_highlight` builtin sets the uniforms in the provided shader for highlighting.
 * This function expects a `shader` reference, and two `string` arguments (the uniform names).
 * The first of the uniforms is used to bind the highlighting FBO texture, and the second is used to determine if this
 * object is the main object to be highlighted.
 */
inline value invoke_enable_highlight(const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  return handle_no_value(
  requires_scope<map_visitor::scope::OBJECTS>("@enable_highlight", cache, pos) >>
    [&args, &pos] { return ready_args<renderer::shader_ref, std::string, std::string>(args, "@enable_highlight", pos); } |
    [&cache]<typename ... Ts>(const std::tuple<Ts...> &tup) {
      const auto &[sh, uniform_tex, uniform_toggle] = tup;
      cache.requires_highlight[sh] = {sh->loc_for(uniform_tex), sh->loc_for(uniform_toggle)};
      return std::monostate{};
    },

    pos
  );
}

/**
 * @brief Invokes the builtin `enable_highlight*` function.
 * @param args The arguments from the parser.
 * @param cache The map visitor cache.
 * @param pos The position of the call.
 * @return A value wrapping `std::monostate`.
 *
 * The `enable_highlight*` builtin sets the uniforms in the provided shader for instanced highlighting.
 * This function expects a `shader` reference, and three `string` arguments (the uniform names).
 * The first of the uniforms is used to bind the highlighting FBO texture, the second is used to determine if this
 * object is the main object to be highlighted, and the third is used to determine the highlighted instance ID.
 */
inline value invoke_enable_highlight_star(const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  return handle_no_value(
  requires_scope<map_visitor::scope::OBJECTS>("@enable_highlight*", cache, pos) >>
    [&args, &pos] { return ready_args<renderer::shader_ref, std::string, std::string, std::string>(args, "@enable_highlight*", pos); } |
    [&cache]<typename ... Ts>(const std::tuple<Ts...> &tup) {
      const auto &[sh, uniform_tex, uniform_toggle, uniform_highlight_id] = tup;
      cache.requires_instanced_highlight[sh] = {sh->loc_for(uniform_tex), sh->loc_for(uniform_toggle), sh->loc_for(uniform_highlight_id)};
      return std::monostate{};
    },

    pos
  );
}

/**
 * @brief Invokes the builtin `highlight_bind` function.
 * @param args The arguments from the parser.
 * @param cache The map visitor cache.
 * @param pos The position of the call.
 * @return A value wrapping `std::monostate`.
 *
 * The `highlight_bind` builtin sets the texture slot to which the highlighting FBO texture is bound.
 * This function expects a single `int` argument.
 */
inline value invoke_highlight_bind(const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  return handle_no_value(
  requires_scope<map_visitor::scope::OBJECTS>("@object", cache, pos) >>
    [&args, &pos] { return ready_arg<int>(args, "@highlight_bind", pos); } |
    [&cache](const int idx) {
      cache.highlight_binding = idx;
      return std::monostate{};
    },

    pos
  );
}

/**
 * @brief Invokes the builtin `add_collider` function.
 * @param args The arguments from the parser.
 * @param v The map visitor.
 * @param pos The position of the call.
 * @return A value wrapping `std::monostate`.
 *
 * The `add_collider` builtin sets the collider for the provided renderable object.
 * This function expects a `renderable` reference, and a `collider` reference.
 */
inline value invoke_add_collider(const std::vector<value> &args, map_visitor &v, const loc &pos) {
  return handle_no_value(
  requires_scope<map_visitor::scope::OBJECTS>("@add_collider", v, pos) >>
    [&args, &pos] { return ready_args<renderer::render_ref, renderer::collider_ref>(args, "@add_collider", pos); } |
    []<typename ... Ts>(const std::tuple<Ts...> &tup) {
      const auto &[rr, coll] = tup;
      rr->coll = coll;
      return std::monostate{};
    },

    pos
  );
}

/**
 * @brief Invokes the builtin `add_collider*` function.
 * @param args The arguments from the parser.
 * @param v The map visitor.
 * @param pos The position of the call.
 * @return A value wrapping `std::monostate`.
 *
 * The `add_collider*` builtin sets the collider for the provided instanced renderable object.
 * This function expects a `instanced_renderable` reference, and a `instanced_collider` reference.
 */
inline value invoke_add_collider_star(const std::vector<value> &args, map_visitor &v, const loc &pos) {
  return handle_no_value(
  requires_scope<map_visitor::scope::OBJECTS>("@collider*", v, pos) >>
    [&args, &pos] { return ready_args<renderer::instanced_render_ref, renderer::instanced_collider_ref>(args, "@add_collider*", pos); } |
    []<typename ... Ts>(const std::tuple<Ts...> &tup) {
      const auto &[rr, coll] = tup;
      rr->coll = coll;
      return std::monostate{};
    },

    pos
  );
}

/**
 * @brief Invokes the builtin `mix` function.
 * @param args The arguments from the parser.
 * @param v The map visitor.
 * @param pos The position of the call.
 * @return Either a voxel corner, or an invalid voxel corner.
 *
 * The `mix` builtin creates a voxel corner from the provided background color, spot color, and blending factor.
 * This function expects two `vec3` arguments and a `float` argument.
 */
inline value invoke_mix(const std::vector<value> &args, map_visitor &v, const loc &pos) {
  return handle(
    requires_scope<map_visitor::scope::VOXEL>("@mix", v, pos) >>
    [&args, &pos] { return ready_args<glm::vec3, glm::vec3, float>(args, "@mix", pos); },
    pos, voxel_corner{}
  );
}

constexpr voxel_desc from_current(const map_visitor &cache) {
  return voxel_desc {
    std::tuple{ cache.voxel_in_progress.back[0], cache.voxel_in_progress.spot[0], cache.voxel_in_progress.fac[0] },
    std::tuple{ cache.voxel_in_progress.back[1], cache.voxel_in_progress.spot[1], cache.voxel_in_progress.fac[1] },
    std::tuple{ cache.voxel_in_progress.back[2], cache.voxel_in_progress.spot[2], cache.voxel_in_progress.fac[2] },
    std::tuple{ cache.voxel_in_progress.back[3], cache.voxel_in_progress.spot[3], cache.voxel_in_progress.fac[3] },
    std::tuple{ cache.voxel_in_progress.back[4], cache.voxel_in_progress.spot[4], cache.voxel_in_progress.fac[4] },
    std::tuple{ cache.voxel_in_progress.back[5], cache.voxel_in_progress.spot[5], cache.voxel_in_progress.fac[5] },
    std::tuple{ cache.voxel_in_progress.back[6], cache.voxel_in_progress.spot[6], cache.voxel_in_progress.fac[6] },
    std::tuple{ cache.voxel_in_progress.back[7], cache.voxel_in_progress.spot[7], cache.voxel_in_progress.fac[7] },
    std::tuple{ cache.voxel_in_progress.back[8], cache.voxel_in_progress.spot[8], cache.voxel_in_progress.fac[8] }
  };
}

/**
 * @brief Invokes the builtin `all` function.
 * @param args The arguments from the parser.
 * @param cache The map visitor cache.
 * @param pos The position of the call.
 * @return Either a voxel descriptor, or an invalid voxel descriptor.
 *
 * The `all` builtin creates a voxel description from the provided voxel corner.
 * This function expects a single `voxel_corner` argument.
 * Each of the 9 voxel corners (center and 8 surrounding points) is set to the same value.
 */
inline value invoke_all(const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  return handle(
  requires_scope<map_visitor::scope::VOXEL>("@all", cache, pos) >>
    [&args, &pos] { return ready_arg<voxel_corner>(args, "@all", pos); } |
    [&cache](const voxel_corner &crn) {
      const auto &[back, spot, fac] = crn;
      for (int i = 0; i < 9; i++) {
        cache.voxel_in_progress.back[i] = back;
        cache.voxel_in_progress.spot[i] = spot;
        cache.voxel_in_progress.fac[i] = fac;
      }

      return from_current(cache);
    },

    pos, voxel_desc{}
  );
}

const static std::unordered_map<std::string, int> indices {
          {"tl", 0}, {"tc", 1}, {"tr", 2},
          {"cl", 3}, {"c",  4}, {"cr", 5},
          {"bl", 6}, {"bc", 7}, {"br", 8}
};

inline value invoke_corner(const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  return handle(
    requires_scope<map_visitor::scope::VOXEL>("@corner", cache, pos) >>
    [&args, &pos] { return ready_args<std::string, voxel_corner>(args, "@corner", pos); } >>
    [&cache, &pos](const std::pair<std::string, voxel_corner> &desc) -> or_error<voxel_desc> {
      const auto &[which, corner] = desc;
      const auto &[back, spot, fac] = corner;
      if (const auto it = indices.find(which); it != indices.end()) {
        cache.voxel_in_progress.back[it->second] = back;
        cache.voxel_in_progress.spot[it->second] = spot;
        cache.voxel_in_progress.fac[it->second] = fac;
        return right(from_current(cache));
      }
      return left(std::format("Invalid corner descriptor '{}' at {}.", which, pos.str()));
    },

    pos, voxel_desc{}
  );
}

inline value invoke_get_corner(const std::vector<value> &args, map_visitor &v, const loc &pos) {
  return handle(
    requires_scope<map_visitor::scope::VOXEL>("@get_corner", v, pos) >>
    [&args, &pos] { return ready_args<std::string, voxel_desc>(args, "@get_corner", pos); } >>
    [&pos](const std::pair<std::string, voxel_desc> &as) -> or_error<voxel_corner> {
      const auto &[which, desc] = as;
      if (const auto it = indices.find(which); it != indices.end()) {
        return right(desc[it->second]);
      }
      return left(std::format("Invalid corner descriptor '{}' at {}.", which, pos.str()));
    },

    pos, voxel_corner{}
  );
}

/**
 * @brief Invokes the builtin `perlin` function.
 * @param args The arguments from the parser.
 * @param cache The map visitor cache.
 * @param pos The position of the call.
 * @return A value wrapping `std::monostate`.
 *
 * The `perlin` builtin sets the alpha and beta values for the perlin noise generation.
 * This function expects 4 sets of `(float, float)` arguments.
 *
 * The alpha values are used to set the influence of each octave on the final noise value.
 * The beta values are used to scale the octaves.
 * The full formula looks like
 * \f$\frac{\sum_{i=0}^4 \alpha_i * noise(\beta_i * coord + \delta_i)}{\sum_{i=0}^4 \alpha_i}\f$.
 * The `delta` values are set using the `seed` function.
 */
inline value invoke_perlin(const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  return handle_no_value(
  requires_scope<map_visitor::scope::VOXEL>("@perlin", cache, pos) >>
    [&args, &pos] { return ready_arg<std::vector<value>>(args, "@perlin", pos); } >>
    [](const std::vector<value> &vs) {
      return type_check_pair_vector(vs, [](const value_pair &vp) {
        return type_check_multi<float, float>({vp.first(), vp.second()});
      });
    } >>
    [&pos](const std::vector<std::tuple<float, float>> &tups) -> or_error<std::vector<std::tuple<float, float>>> {
      if (tups.size() == 4) return right(tups);
      return left(std::format("@perlin requires 4 sets of arguments, {} given at {}.", tups.size(), pos.str()));
    } |
    [&cache](const std::vector<std::tuple<float, float>> &tups) {
      for (int i = 0; i < 4; i++) {
        cache.voxel_in_progress.alpha[i] = std::get<0>(tups[i]);
        cache.voxel_in_progress.beta[i] = std::get<1>(tups[i]);
      }
      return std::monostate{};
    },

    pos
  );
}

/**
 * @brief Invokes the builtin `seed` function.
 * @param args The arguments from the parser.
 * @param cache The map visitor cache.
 * @param pos The position of the call.
 * @return A value wrapping `std::monostate`.
 *
 * The `seed` builtin sets the delta values for the perlin noise generation.
 * This function expects 4 `float` arguments.
 * The `delta` values are used to set the offset for each octave; @ref openvtt::map::invoke_perlin.
 */
inline value invoke_seed(const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  return handle_no_value(
    requires_scope<map_visitor::scope::VOXEL>("@seed", cache, pos) >>
    [&args, &pos] { return ready_args<float, float, float, float>(args, "@seed", pos); } |
    [&cache](const std::tuple<float, float, float, float> &tup) {
      cache.voxel_in_progress.delta[0] = std::get<0>(tup);
      cache.voxel_in_progress.delta[1] = std::get<1>(tup);
      cache.voxel_in_progress.delta[2] = std::get<2>(tup);
      cache.voxel_in_progress.delta[3] = std::get<3>(tup);
      return std::monostate{};
    },

    pos
  );
}

/**
 * @brief Invokes the builtin `axes` function.
 * @param args The arguments from the parser.
 * @param cache The map visitor cache.
 * @param pos The position of the call.
 * @return A value wrapping `std::monostate`.
 *
 * The `axes` builtin toggles the display of the axes in the map (at the origin).
 * This function expects a single `bool` argument.
 */
inline value invoke_axes(const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  return handle_no_value(
    requires_scope<map_visitor::scope::OBJECTS>("@axes", cache, pos) >>
    [&args, &pos] { return ready_arg<bool>(args, "@axes", pos); } |
    [&cache](const bool &draw) {
      cache.show_axes = draw;
      return std::monostate{};
    }, pos
  );
}

/**
 * @brief Invokes the builtin `region` function.
 * @param args The arguments from the parser.
 * @param v The map visitor.
 * @param pos The position of the call.
 * @return Either a list of coordinates, or an empty list.
 *
 * The `region` builtin creates a list of coordinates from the provided list of integer pairs.
 * This function expects a list of integer pairs (x, y).
 * Each pair is interpreted as point, and two consecutive points are used to create an edge of a polygon. The last and
 * first point are also used to create an edge. The voxels interior to the polygon are returned as a region. It is
 * undefined behavior to pass a list of less than 3 points, or a polygon with intersecting edges (non-consecutive pairs
 * that intersect).
 */
inline value invoke_region(const std::vector<value> &args, map_visitor &v, const loc &pos) {
  return handle(
    requires_scope<map_visitor::scope::VOXEL>("@region", v, pos) >>
    [&args, &pos] { return ready_arg<std::vector<value>>(args, "@region", pos); } >>
    [&pos](const std::vector<value> &pts) -> or_error<std::vector<std::pair<int, int>>> {
      std::vector<std::pair<int, int>> res;
      res.reserve(pts.size());
      for (const auto &x: pts) {
        if (const auto y = x.expecting<int, int>(); y.has_value()) res.push_back(*y);
        else return left("@region expects a list of integer pairs (at " + pos.str());
      }
      return right(res);
    } | [&pos](const std::vector<std::pair<int, int>> &pts) -> std::vector<value> {
      const auto fill = scanline_fill(pts);
      return map_vec(fill, [&pos](const std::pair<int, int> &p) -> value {
        return value{ value_pair{ value{p.first, pos}, value{p.second, pos} }, pos };
      });
    },
    pos,
    std::vector<value>{}
  );
}

/**
 * @brief Invokes the builtin `border` function.
 * @param args The arguments from the parser.
 * @param v The map visitor.
 * @param pos The position of the call.
 * @return Either a list of coordinates, or an empty list.
 *
 * The `border` builtin creates a list of coordinates from the provided list of integer pairs.
 * This function expects a list of integer pairs (x, y) and an integer width.
 * For each point in the list, all points around that point within the specified distance are returned.
 * The output list does not contain any duplicates, nor any points within the original list.
 */
inline value invoke_border(const std::vector<value> &args, map_visitor &v, const loc &pos) {
  return handle(
  requires_scope<map_visitor::scope::VOXEL>("@border", v, pos) >>
    [&args, &pos] { return ready_args<std::vector<value>, int>(args, "@border", pos); } >>
    [&pos](const std::pair<std::vector<value>, int> &a) -> or_error<std::pair<std::vector<std::pair<int, int>>, int>> {
      const auto &[pts, w] = a;
      std::vector<std::pair<int, int>> res;
      res.reserve(pts.size());
      for (const auto &x: pts) {
        if (const auto y = x.expecting<int, int>(); y.has_value()) res.push_back(*y);
        else return left("@region expects a list of integer pairs (at " + pos.str());
      }
      return right(std::pair{res, w});
    } | [&pos](const std::pair<std::vector<std::pair<int, int>>, int> &a) -> std::vector<value> {
      const auto &[a0, a1] = a;
      const auto fill = border(a0, a1);
      return map_vec(fill, [&pos](const std::pair<int, int> &p) -> value {
        return value{ value_pair{ value{p.first, pos}, value{p.second, pos} }, pos };
      });
    },
    pos,
    std::vector<value>{}
  );
}

/**
 * @brief Invokes the builtin `print` function.
 * @param args The arguments from the parser.
 * @param pos The position of the call.
 * @return A value wrapping `std::monostate`.
 *
 * The `print` builtin logs all values passed to it as a single informational message.
 */
inline value invoke_print(const std::vector<value> &args, map_visitor &, const loc &pos) {
  std::stringstream strm;
  if (args.empty()) return value{std::monostate{}, pos};
  strm << static_cast<std::string>(args[0]);
  for (size_t i = 1; i < args.size(); i++) strm << ' ' << static_cast<std::string>(args[i]);
  renderer::log<renderer::log_type::INFO>("@print", "({}) {}", pos.str(), strm.str());
  return value{std::monostate{}, pos};
}

/**
 * @brief The type of builtin functions: (const std::vector<value> &, map_visitor &, const loc &) -> value.
 */
using builtin_f = value (*)(const std::vector<value> &, map_visitor &, const loc &);

/**
 * @brief Invokes the required builtin function, if it exists.
 * @param name The function to be invoked.
 * @param args The arguments to the function.
 * @param cache The map visitor cache.
 * @param pos The position of the call.
 * @return The return value of the function, or `std::monostate` (void) if the function does not exist.
 */
inline value invoke_builtin(const std::string &name, const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  const static std::unordered_map<std::string, builtin_f> builtins {
    {"@object", invoke_object}, {"@object*", invoke_object_star},
    {"@shader", invoke_shader}, {"@texture", invoke_texture},
    {"@collider", invoke_collider}, {"@collider*", invoke_collider_star},
    {"@transform", invoke_transform},
    {"@spawn", invoke_spawn}, {"@spawn*", invoke_spawn_star},
    {"@transform_obj", invoke_transform_obj},
    {"@enable_highlight", invoke_enable_highlight}, {"@enable_highlight*", invoke_enable_highlight_star},
    {"@highlight_bind", invoke_highlight_bind},
    {"@add_collider", invoke_add_collider}, {"@add_collider*", invoke_add_collider_star},
    {"@mix", invoke_mix}, {"@all", invoke_all}, {"@corner", invoke_corner}, {"@get_corner", invoke_get_corner},
    {"@perlin", invoke_perlin}, {"@seed", invoke_seed},
    {"@axes", invoke_axes}, {"@region", invoke_region}, {"@border", invoke_border},
    {"@print", invoke_print}
  };

  const auto it = builtins.find(name);
  if (it == builtins.end()) {
    renderer::log<renderer::log_type::ERROR>("map_loader", std::format("Unknown builtin function {} at {}.", name, pos.str()));
    return value{std::monostate{}, pos};
  }

  return it->second(args, cache, pos);
}
}

#endif //MAP_BUILTINS_HPP
