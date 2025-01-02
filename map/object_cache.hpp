//
// Created by jay on 12/14/24.
//

#ifndef OBJECT_CACHE_HPP
#define OBJECT_CACHE_HPP

#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <glm/glm.hpp>
#include <antlr4-runtime/antlr4-runtime.h>

#include "util.hpp"
#include "renderer/render_cache.hpp"
#include "renderer/log_view.hpp"

namespace openvtt::map {
class loc {
public:
  constexpr loc() : file{"(invalid)"}, line{-1UL}, col{-1UL} {}
  constexpr loc(std::string file, const size_t line, const size_t col) : file{std::move(file)}, line{line}, col{col} {}
  constexpr loc(const antlr4::ParserRuleContext &ctx, std::string file) :
    file{std::move(file)}, line{ctx.start->getLine()}, col{ctx.start->getCharPositionInLine()} {}

  [[nodiscard]] constexpr std::string str() const { return std::format("{}:{}:{}", file, line, col); }

private:
  std::string file{};
  size_t line;
  size_t col;
};

class value;

class value_pair {
public:
  value_pair();
  value_pair(const value &first, const value &second);
  value_pair(value &&first, value &&second);
  value_pair(const value_pair &other);
  value_pair(value_pair &&other) noexcept;

  value_pair &operator=(const value_pair &other);
  value_pair &operator=(value_pair &&other) noexcept;

  [[nodiscard]] std::pair<const value &, const value &> as_pair() const;
  [[nodiscard]] value &first() const;
  [[nodiscard]] value &second() const;

  ~value_pair();
private:
  value *_first = nullptr;
  value *_second = nullptr;
};

using voxel_corner = std::tuple<glm::vec3, glm::vec3, float>;
using voxel_desc = std::array<voxel_corner, 9>;

template <typename T>
concept valid_value = std::same_as<T, int> || std::same_as<T, float> || std::same_as<T, std::string> ||
  std::same_as<T, glm::vec3>  || std::same_as<T, glm::mat4> || std::same_as<T, renderer::object_ref> ||
  std::same_as<T, renderer::instanced_object_ref> || std::same_as<T, renderer::shader_ref> ||
  std::same_as<T, renderer::texture_ref> || std::same_as<T, renderer::collider_ref> ||
  std::same_as<T, renderer::instanced_collider_ref> || std::same_as<T, renderer::render_ref> ||
  std::same_as<T, renderer::instanced_render_ref> || std::same_as<T, value_pair> ||
  std::same_as<T, voxel_corner> || std::same_as<T, voxel_desc> ||
  std::same_as<T, std::vector<value>> || std::same_as<T, std::monostate>;

template <valid_value T>
constexpr std::string type_name() {
  if constexpr(std::same_as<T, int>) return "int";
  if constexpr(std::same_as<T, float>) return "float";
  if constexpr(std::same_as<T, std::string>) return "string";
  if constexpr(std::same_as<T, glm::vec3>) return "vec3";
  if constexpr(std::same_as<T, glm::mat4>) return "mat4[transform]";
  if constexpr(std::same_as<T, renderer::object_ref>) return "object";
  if constexpr(std::same_as<T, renderer::instanced_object_ref>) return "instanced_object";
  if constexpr(std::same_as<T, renderer::shader_ref>) return "shader";
  if constexpr(std::same_as<T, renderer::texture_ref>) return "texture";
  if constexpr(std::same_as<T, renderer::collider_ref>) return "collider";
  if constexpr(std::same_as<T, renderer::instanced_collider_ref>) return "instanced_collider";
  if constexpr(std::same_as<T, renderer::render_ref>) return "renderable";
  if constexpr(std::same_as<T, renderer::instanced_render_ref>) return "instanced_renderable";
  if constexpr(std::same_as<T, value_pair>) return "pair";
  if constexpr(std::same_as<T, voxel_corner>) return "voxel_corner";
  if constexpr(std::same_as<T, voxel_desc>) return "voxel_desc";
  if constexpr(std::same_as<T, std::vector<value>>) return "list";
  if constexpr(std::same_as<T, std::monostate>) return "void";
  OPENVTT_UNREACHABLE;
}

class value {
public:
  constexpr value() = default;
  template <valid_value T>
  constexpr explicit value(T x, loc at) : x{x}, generated{std::move(at)} {}

  template <valid_value T>
  constexpr const T &as() const { return std::get<T>(x); }
  template <valid_value T>
  constexpr T &as() { return std::get<T>(x); }
  template <valid_value T>
  [[nodiscard]] constexpr bool is() const { return !x.valueless_by_exception() && std::holds_alternative<T>(x); }
  template <typename F>
  constexpr auto visit(F &&f) { return std::visit(f, x); }

  [[nodiscard]] constexpr std::string type_name() const {
    if (x.valueless_by_exception()) {
      return "(invalid type; no value)";
    }
    return std::visit([]<typename T>(const T &){ return map::type_name<T>(); }, x);
  }

  template <valid_value T>
  [[nodiscard]] inline T should_be() const;

  [[nodiscard]] constexpr const loc &pos() const { return generated; }

private:
  using var_t = std::variant<
    int, float, std::string, glm::vec3, glm::mat4,
    renderer::object_ref, renderer::instanced_object_ref, renderer::shader_ref, renderer::texture_ref,
    renderer::collider_ref, renderer::instanced_collider_ref, renderer::render_ref, renderer::instanced_render_ref,
    voxel_corner, voxel_desc,
    value_pair, std::vector<value>, std::monostate
  >;

  var_t x;
  loc generated;
};

template <valid_value T> struct default_value;
template <> struct default_value<int> { static constexpr int value = 0; };
template <> struct default_value<float> { static constexpr float value = 0.0f; };
template <> struct default_value<std::string> { static constexpr std::string value{}; };
template <> struct default_value<glm::vec3> { static constexpr auto value = glm::vec3{0.0f}; };
template <> struct default_value<glm::mat4> { static constexpr auto value = glm::mat4{1.0f}; };
template <> struct default_value<renderer::object_ref> { static constexpr renderer::object_ref value = renderer::object_ref::invalid(); };
template <> struct default_value<renderer::instanced_object_ref> { static constexpr renderer::instanced_object_ref value = renderer::instanced_object_ref::invalid(); };
template <> struct default_value<renderer::shader_ref> { static constexpr renderer::shader_ref value = renderer::shader_ref::invalid(); };
template <> struct default_value<renderer::texture_ref> { static constexpr renderer::texture_ref value = renderer::texture_ref::invalid(); };
template <> struct default_value<renderer::collider_ref> { static constexpr renderer::collider_ref value = renderer::collider_ref::invalid(); };
template <> struct default_value<renderer::instanced_collider_ref> { static constexpr renderer::instanced_collider_ref value = renderer::instanced_collider_ref::invalid(); };
template <> struct default_value<renderer::render_ref> { static constexpr renderer::render_ref value = renderer::render_ref::invalid(); };
template <> struct default_value<renderer::instanced_render_ref> { static constexpr renderer::instanced_render_ref value = renderer::instanced_render_ref::invalid(); };
template <> struct default_value<value_pair> { static inline value_pair value{}; };
template <> struct default_value<voxel_corner> { static constexpr voxel_corner value{}; };
template <> struct default_value<voxel_desc> { static constexpr voxel_desc value{}; };
template <> struct default_value<std::vector<value>> { static constexpr std::vector<openvtt::map::value> value{}; };
template <> struct default_value<std::monostate> { static constexpr std::monostate value{}; };

template <valid_value T>
inline T default_value_v = default_value<T>::value;

template<valid_value T>
T value::should_be() const {
  if (is<T>()) return as<T>();
  if constexpr(std::same_as<T, float>) {
    if (is<int>()) return static_cast<float>(as<int>());
  }

  renderer::log<renderer::log_type::ERROR>("object_cache",
    std::format("Expected value of type {}, but got value of type {} at {}",
      map::type_name<T>(), type_name(), generated.str()
    )
  );
  return default_value_v<T>;
}


class object_cache {
public:
  std::optional<value> lookup_var_maybe(const std::string &name) const {
    const auto it = ctx.find(name);
    return it == ctx.end() ? std::nullopt : std::optional{it->second.first};
  }

  std::optional<value> lookup_var_maybe(const std::string &name, const loc &at) const {
    return with_empty(lookup_var_maybe(name), [&at, &name]() {
      renderer::log<renderer::log_type::ERROR>("object_cache",
        std::format("Variable {} does not exist at {}", name, at.str())
      );
    });
  }

  value lookup_var(const std::string &name, const loc &at) const {
    return lookup_var_maybe(name, at) || [] { return value{}; };
  }

  template <valid_value T>
  std::optional<T> lookup_maybe(const std::string &name) {
    return lookup_var_maybe(name) >> [](const std::pair<value, loc> &v) {
      if (v.first.is<T>()) return std::optional{v.first.as<T>()};
      return std::nullopt;
    };
  }

  template <valid_value T>
  T lookup(const std::string &name, const loc &at) {
    const auto var = lookup_var(name, at);
    if (var.is<T>()) return var.as<T>();
    renderer::log<renderer::log_type::ERROR>("object_cache",
      std::format("Variable {} is of type {}, expected {} at {}",
        name, var.type_name(), type_name<T>(), at.str()
      )
    );
    return default_value_v<T>;
  }

  template <valid_value T>
  void assign(const std::string &name, T val, const loc &at) {
    if (const auto it = ctx.find(name); it != ctx.end()) {
      if (const auto &[old, declared] = it->second; old.is<T>()) {
        ctx[name] = {value{std::move(val), at}, at};
      }
      else {
        renderer::log<renderer::log_type::ERROR>("object_cache",
          std::format("Variable {} is of type {}, but got assigned {} at {}",
            name, old.type_name(), type_name<T>(), at.str()
          )
        );
      }
    }
    else {
      ctx[name] = {value{std::forward<T>(val), at}, at};
    }
  }

  void assign(const std::string &name, value &&val, const loc &at) {
    val.visit([this, &name, &at]<typename T>(T &&t) { assign(name, t, at); });
  }

private:
  std::unordered_map<std::string, std::pair<value, loc>> ctx;
};
}

#endif //OBJECT_CACHE_HPP
