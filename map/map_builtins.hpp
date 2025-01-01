//
// Created by jay on 1/1/25.
//

#ifndef MAP_BUILTINS_HPP
#define MAP_BUILTINS_HPP

#include "either.hpp"
#include "object_cache.hpp"
#include "map_visitor.hpp"
#include "renderer/log_view.hpp"

namespace openvtt::map {
template <typename T> using or_error = either<std::string, T>;

inline or_error<std::vector<value>> n_args(const std::vector<value> &args, const size_t n, const std::string &function, const loc &pos) {
  if (args.size() != n) return left(std::format("Function {} expects {} arguments, but got {} at {}.", function, n, args.size(), pos.str()));
  return right(args);
}

template <valid_value T>
inline or_error<T> type_check(const value &v) {
  if (v.is<T>()) return right(v.as<T>());
  return left(std::format("Expected a value of type {}, but got {} at {}.", type_name<T>(), v.type_name(), v.pos().str()));
}

template <valid_value ... Ts>
or_error<std::tuple<Ts...>> type_check_multi(const std::array<value, sizeof...(Ts)> &vs) {
  using tup = std::tuple<Ts...>;
  constexpr auto lambda = []<size_t ... idxs>(const std::array<value, sizeof...(Ts)> &v, std::index_sequence<idxs...>) {
    return merge(type_check<std::tuple_element_t<idxs, tup>>(v[idxs])...);
  };

  return lambda(vs, std::make_index_sequence<sizeof...(Ts)>{});
}

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

template <std::invocable<const value_pair &> F>
requires(is_either<res_t<F, const value_pair &>>)
inline or_error<std::vector<typename either_traits<res_t<F, const value_pair &>>::right_t>> type_check_vector(const std::vector<value> &vs, F &&f) {
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

template <valid_value T>
or_error<T> ready_arg(const std::vector<value> &args, const std::string &function, const loc &pos) {
  return n_args(args, 1, function, pos) >> [](const std::vector<value> &a) { return type_check<T>(a[0]); };
}

template <valid_value ... Ts>
or_error<std::tuple<Ts...>> ready_args(const std::vector<value> &args, const std::string &function, const loc &pos) {
  constexpr static auto lambda = []<size_t ... idxs>(const std::vector<value> &a, std::index_sequence<idxs...>) {
    return type_check_multi<Ts...>({a[idxs]...});
  };

  return n_args(args, sizeof...(Ts), function, pos)
    >> [](const std::vector<value> &a) { return lambda(a, std::make_index_sequence<sizeof...(Ts)>{}); };
}

template <valid_value T>
value handle(const or_error<T> &res, const loc &pos, const T &backup = {}) {
  return fold(
    map_left(res, [&backup](const std::string &err){ renderer::log<renderer::log_type::WARNING>("map_loader", err); return backup; }),
    [&pos](const T &x) -> value { return value{x, pos}; },
    [&pos](const T &x) -> value { return value{x, pos}; }
  );
}

template <typename T>
no_value handle_no_value(const or_error<T> &res, const loc &pos) {
  map_left(res, [](const std::string &err) { renderer::log<renderer::log_type::WARNING>("map_loader", err); return std::monostate{}; });
  return no_value{pos};
}

inline value invoke_object(const std::vector<value> &args, map_visitor &, const loc &pos) {
  return handle(
    ready_arg<std::string>(args, "@object", pos) |
    [](const std::string &asset) { return renderer::render_cache::load<renderer::render_object>(asset); },

    pos, renderer::object_ref::invalid());
}

inline value invoke_object_star(const std::vector<value> &args, map_visitor &, const loc &pos) {
  return handle(
    ready_args<std::string, std::vector<value>>(args, "@object*", pos) >>
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

inline value invoke_shader(const std::vector<value> &args, map_visitor &, const loc &pos) {
  return handle(
    ready_args<std::string, std::string>(args, "@shader", pos) |
    [](const std::tuple<std::string, std::string> &vf) { return renderer::render_cache::load<renderer::shader>(std::get<0>(vf), std::get<1>(vf)); },

    pos, renderer::shader_ref::invalid()
  );
}

inline value invoke_texture(const std::vector<value> &args, map_visitor &, const loc &pos) {
  return handle(
    ready_arg<std::string>(args, "@texture", pos) |
    [](const std::string &asset) { return renderer::render_cache::construct<renderer::texture>(asset); },

    pos, renderer::texture_ref::invalid()
  );
}

inline value invoke_collider(const std::vector<value> &args, map_visitor &, const loc &pos) {
  return handle(
    ready_arg<std::string>(args, "@collider", pos) |
    [](const std::string &asset) { return renderer::render_cache::load<renderer::collider>(asset); },

    pos, renderer::collider_ref::invalid()
  );
}

inline value invoke_collider_star(const std::vector<value> &args, map_visitor &, const loc &pos) {
  return handle(
    ready_args<std::string, std::vector<value>>(args, "@collider*", pos) >>
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

inline value invoke_transform(const std::vector<value> &args, map_visitor &, const loc &pos) {
  return handle(
    ready_args<glm::vec3, glm::vec3, glm::vec3>(args, "@transform", pos) |
    [](const std::tuple<glm::vec3, glm::vec3, glm::vec3> &tup) {
      const auto &[pos, rot, scale] = tup;
      return renderer::instanced_object::model_for(rot, scale, pos);
    },

    pos, glm::mat4(1.0f)
  );
}

inline value invoke_spawn(const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  return handle(
    // check arguments
    ready_args<std::string, renderer::object_ref, renderer::shader_ref, std::vector<value>>(args, "@spawn", pos) >>
    [&cache](const auto &a) -> or_error<renderer::render_ref> {
      // check textures
      const auto &[name, obj, sh, textures] = a;

      return type_check_vector(textures, [](const value_pair &vp) {
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

inline value invoke_spawn_star(const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  return handle(
    // check arguments
    ready_args<std::string, renderer::instanced_object_ref, renderer::shader_ref, std::vector<value>>(args, "@spawn*", pos) >>
    [](const auto &a) -> or_error<renderer::instanced_render_ref> {
      // check textures
      const auto &[asset, obj, sh, textures] = a;
      return type_check_vector(textures, [](const value_pair &vp) {
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

inline no_value invoke_transform_obj(const std::vector<value> &args, map_visitor &, const loc &pos) {
  return handle_no_value(
    ready_args<renderer::render_ref, glm::vec3, glm::vec3, glm::vec3>(args, "@transform_obj", pos) |
    []<typename ... Ts>(const std::tuple<Ts...> &tup) {
      const auto &[rr, p, r, s] = tup;
      rr->position = p; rr->rotation = r; rr->scale = s;
      return std::monostate{};
    },

    pos
  );
}

inline no_value invoke_enable_highlight(const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  return handle_no_value(
    ready_args<renderer::shader_ref, std::string, std::string>(args, "@enable_highlight", pos) |
    [&cache]<typename ... Ts>(const std::tuple<Ts...> &tup) {
      const auto &[sh, uniform_tex, uniform_toggle] = tup;
      cache.requires_highlight[sh] = {sh->loc_for(uniform_tex), sh->loc_for(uniform_toggle)};
      return std::monostate{};
    },

    pos
  );
}

inline no_value invoke_enable_highlight_star(const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  return handle_no_value(
    ready_args<renderer::shader_ref, std::string, std::string, std::string>(args, "@enable_highlight*", pos) |
    [&cache]<typename ... Ts>(const std::tuple<Ts...> &tup) {
      const auto &[sh, uniform_tex, uniform_toggle, uniform_highlight_id] = tup;
      cache.requires_instanced_highlight[sh] = {sh->loc_for(uniform_tex), sh->loc_for(uniform_toggle), sh->loc_for(uniform_highlight_id)};
      return std::monostate{};
    },

    pos
  );
}

inline no_value invoke_highlight_bind(const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  return handle_no_value(
    ready_arg<int>(args, "@highlight_bind", pos) |
    [&cache](const int idx) {
      cache.highlight_binding = idx;
      return std::monostate{};
    },

    pos
  );
}

inline no_value invoke_add_collider(const std::vector<value> &args, map_visitor &, const loc &pos) {
  return handle_no_value(
    ready_args<renderer::render_ref, renderer::collider_ref>(args, "@add_collider", pos) |
    []<typename ... Ts>(const std::tuple<Ts...> &tup) {
      const auto &[rr, coll] = tup;
      rr->coll = coll;
      return std::monostate{};
    },

    pos
  );
}

inline no_value invoke_add_collider_star(const std::vector<value> &args, map_visitor &, const loc &pos) {
  return handle_no_value(
    ready_args<renderer::instanced_render_ref, renderer::instanced_collider_ref>(args, "@add_collider*", pos) |
    []<typename ... Ts>(const std::tuple<Ts...> &tup) {
      const auto &[rr, coll] = tup;
      rr->coll = coll;
      return std::monostate{};
    },

    pos
  );
}

enum struct builtin {
  LD_OBJECT, LD_OBJECT_STAR, LD_SHADER, LD_TEXTURE, LD_COLLIDER, LD_COLLIDER_STAR, LD_TRANSFORM,
  SPAWN, SPAWN_STAR, TRANSFORM_OBJ, ENABLE_HIGHLIGHT, ENABLE_HIGHLIGHT_STAR, HIGHLIGHT_BIND,
  ADD_COLLIDER, ADD_COLLIDER_STAR
};

inline std::any invoke_builtin(const std::string &name, const std::vector<value> &args, map_visitor &cache, const loc &pos) {
  using enum builtin;
  const static std::unordered_map<std::string, builtin> builtins {
    {"@object", LD_OBJECT}, {"@object*", LD_OBJECT_STAR}, {"@shader", LD_SHADER}, {"@texture", LD_TEXTURE},
    {"@collider", LD_COLLIDER}, {"@collider*", LD_COLLIDER_STAR}, {"@transform", LD_TRANSFORM},
    {"@spawn", SPAWN}, {"@spawn*", SPAWN_STAR}, {"@transform_obj", TRANSFORM_OBJ},
    {"@enable_highlight", ENABLE_HIGHLIGHT}, {"@enable_highlight*", ENABLE_HIGHLIGHT_STAR},
    {"@highlight_bind", HIGHLIGHT_BIND},
    {"@add_collider", ADD_COLLIDER}, {"@add_collider*", ADD_COLLIDER_STAR}
  };

  const auto it = builtins.find(name);
  if (it == builtins.end()) {
    renderer::log<renderer::log_type::WARNING>("map_loader", std::format("Unknown builtin function {} at {}.", name, pos.str()));
    return no_value{pos};
  }

  switch (it->second) {
    case LD_OBJECT: return invoke_object(args, cache, pos);
    case LD_OBJECT_STAR: return invoke_object_star(args, cache, pos);
    case LD_SHADER: return invoke_shader(args, cache, pos);
    case LD_TEXTURE: return invoke_texture(args, cache, pos);
    case LD_COLLIDER: return invoke_collider(args, cache, pos);
    case LD_COLLIDER_STAR: return invoke_collider_star(args, cache, pos);
    case LD_TRANSFORM: return invoke_transform(args, cache, pos);
    case SPAWN: return invoke_spawn(args, cache, pos);
    case SPAWN_STAR: return invoke_spawn_star(args, cache, pos);
    case TRANSFORM_OBJ: return invoke_transform_obj(args, cache, pos);
    case ENABLE_HIGHLIGHT: return invoke_enable_highlight(args, cache, pos);
    case ENABLE_HIGHLIGHT_STAR: return invoke_enable_highlight_star(args, cache, pos);
    case HIGHLIGHT_BIND: return invoke_highlight_bind(args, cache, pos);
    case ADD_COLLIDER: return invoke_add_collider(args, cache, pos);
    case ADD_COLLIDER_STAR: return invoke_add_collider_star(args, cache, pos);

    default: std::unreachable();
  }
}
}

#endif //MAP_BUILTINS_HPP
