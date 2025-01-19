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
/**
 * @brief Structure representing a location in the source file.
 */
class loc {
public:
  /**
   * @brief Constructs a new (invalid) location.
   */
  constexpr loc() : file{"(invalid)"}, line{-1UL}, col{-1UL} {}
  /**
   * @brief Constructs a new location.
   * @param file The file name.
   * @param line The line number.
   * @param col The column number.
   */
  constexpr loc(std::string file, const size_t line, const size_t col) : file{std::move(file)}, line{line}, col{col} {}

  /**
   * @brief Constructs a new location from a parser context.
   * @param ctx The parser context.
   * @param file The file name.
   *
   * The context is used to extract the line number and column number (both from the start of the context).
   */
  constexpr loc(const antlr4::ParserRuleContext &ctx, std::string file) :
    file{std::move(file)}, line{ctx.start->getLine()}, col{ctx.start->getCharPositionInLine() + 1} {}

  /**
   * @brief Generates a string representation of the location.
   * @return The string representation of the location.
   */
  [[nodiscard]] constexpr std::string str() const { return std::format("{}:{}:{}", file, line, col); }

private:
  std::string file{};
  size_t line;
  size_t col;
};

class value;

/**
 * @brief Structure representing a pair of values.
 *
 * Due to cyclical dependencies, the two values are stored as pointers (on the heap).
 */
class value_pair {
public:
  /**
   * @brief Creates a new value pair with two invalid values.
   */
  value_pair();

  /**
   * @brief Creates a new value pair by copying the given values.
   * @param first The first value.
   * @param second The second value.
   */
  value_pair(const value &first, const value &second);

  /**
   * @brief Creates a new value pair by moving the given values.
   * @param first The first value.
   * @param second The second value.
   */
  value_pair(value &&first, value &&second);
  value_pair(const value_pair &other);
  value_pair(value_pair &&other) noexcept;

  value_pair &operator=(const value_pair &other);
  value_pair &operator=(value_pair &&other) noexcept;

  /**
   * @brief Gets the first and second values.
   * @return A `std::pair` containing references to the first and second values.
   */
  [[nodiscard]] std::pair<const value &, const value &> as_pair() const;

  /**
   * @brief Gets a reference to the first value.
   * @return The first value.
   */
  [[nodiscard]] value &first() const;

  /**
   * @brief Gets a reference to the second value.
   * @return The second value.
   */
  [[nodiscard]] value &second() const;

  [[nodiscard]] bool operator==(const value_pair &other) const;

  ~value_pair();
private:
  value *_first = nullptr;
  value *_second = nullptr;
};

using voxel_corner = std::tuple<glm::vec3, glm::vec3, float>; //!< Type alias for a voxel corner `(glm::vec3, glm::vec3, float)`.
using voxel_desc = std::array<voxel_corner, 9>; //!< Type alias for a voxel description (`std::array<voxel_corner, 9>`).

/**
 * @brief Concept representing a valid value type.
 */
template <typename T>
concept valid_value = std::same_as<T, bool> || std::same_as<T, int> || std::same_as<T, float> || std::same_as<T, std::string> ||
  std::same_as<T, glm::vec3>  || std::same_as<T, glm::mat4> || std::same_as<T, renderer::object_ref> ||
  std::same_as<T, renderer::instanced_object_ref> || std::same_as<T, renderer::shader_ref> ||
  std::same_as<T, renderer::texture_ref> || std::same_as<T, renderer::collider_ref> ||
  std::same_as<T, renderer::instanced_collider_ref> || std::same_as<T, renderer::render_ref> ||
  std::same_as<T, renderer::instanced_render_ref> || std::same_as<T, value_pair> ||
  std::same_as<T, voxel_corner> || std::same_as<T, voxel_desc> ||
  std::same_as<T, std::vector<value>> || std::same_as<T, std::monostate>;

/**
 * @brief Gets a string representation of the type of the value.
 * @tparam T The type of the value.
 * @return The string representation of the type of the value.
 */
template <valid_value T>
constexpr std::string type_name() {
  if constexpr(std::same_as<T, bool>) return "bool";
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

/**
 * @brief Structure representing a value.
 *
 * Values are stored together with the location in the source file where they were generated.
 *
 * The following types are supported as values:
 * - `int`, `float`, `std::string` primitive types,
 * - `glm::vec3`, `glm::mat4` vector and matrix types,
 * - `renderer::object_ref`, `renderer::instanced_object_ref`, `renderer::shader_ref`, `renderer::texture_ref`,
 * `renderer::collider_ref`, `renderer::instanced_collider_ref`, `renderer::render_ref`,
 * `renderer::instanced_render_ref` types from the renderer,
 * - `pair` (a pair of two values),
 * - `voxel_corner` and `voxel_desc` types for voxels,
 * - `std::vector<value>` for lists of values,
 * - `std::monostate` as `void` alias (representing no value).
 */
class value {
public:
  /**
   * @brief Constructs a new (invalid/default) value.
   */
  constexpr value() = default;

  /**
   * @brief Copies a concrete value into a new value.
   * @tparam T The type of the value.
   * @param x The value to copy.
   * @param at The location of the value.
   */
  template <valid_value T>
  constexpr explicit value(T x, loc at) : x{x}, generated{std::move(at)} {}

  /**
   * @brief Force-casts a value to a given type.
   * @tparam T The type to cast to.
   * @return The cast value.
   */
  template <valid_value T>
  constexpr const T &as() const { return std::get<T>(x); }
  /**
   * @brief Force-casts a value to a given type.
   * @tparam T The type to cast to.
   * @return The cast value.
   */
  template <valid_value T>
  constexpr T &as() { return std::get<T>(x); }

  /**
   * @brief Checks if the value is of a given type.
   * @tparam T The type to check for.
   * @return `true` if the value is of the given type, `false` otherwise.
   *
   * If the contained value (`std::variant`) would be `valueless_by_exception`, this function returns `false`.
   */
  template <valid_value T>
  [[nodiscard]] inline bool is() const { return !x.valueless_by_exception() && std::holds_alternative<T>(x); }

  /**
   * @brief Applies a visitor to the value.
   * @tparam F The type of the visitor.
   * @param f The visitor to apply.
   * @return The return value of the visitor.
   */
  template <typename F>
  constexpr auto visit(F &&f) { return std::visit(f, x); }

  /**
   * @brief Gets a string representation of the type of the value.
   * @return The string representation of the type of the value.
   */
  [[nodiscard]] constexpr std::string type_name() const {
    if (x.valueless_by_exception()) {
      return "(invalid type; no value)";
    }
    return std::visit([]<typename T>(const T &){ return map::type_name<T>(); }, x);
  }

  template <valid_value T>
  [[nodiscard]] constexpr std::optional<T> cast_maybe() const {
    if (is<T>()) return as<T>();
    if constexpr (std::same_as<T, float>) {
      if (is<int>()) return static_cast<float>(as<int>());
    }
    return std::nullopt;
  }

  template <valid_value T>
  [[nodiscard]] inline T should_be() const;

  /**
   * @brief Attempts to force a value into a certain type.
   * @tparam T The target type.
   * @return The value as the target type, or the default value for that type.
   *
   * If the value is not of the target type, an error message is logged, and `std::nullopt` is returned.
   * Otherwise, the value is returned as the target type.
   */
  template <valid_value T>
  [[nodiscard]] constexpr std::optional<T> expecting() const {
    if (is<T>()) return as<T>();
    if constexpr (std::same_as<T, float>) {
      if (is<int>()) return static_cast<float>(as<int>());
    }

    renderer::log<renderer::log_type::ERROR>("object_cache",
      std::format("Expected value of type {}, but got value of type {} at {}",
        map::type_name<T>(), type_name(), generated.str()
      )
    );
    return std::nullopt;
  }

  template <valid_value T1, valid_value T2>
  [[nodiscard]] constexpr std::optional<std::pair<T1, T2>> expecting() const {
    if (is<value_pair>()) {
      const auto &[x, y] = as<value_pair>().as_pair();
      const auto x_m = x.cast_maybe<T1>();
      const auto y_m = y.cast_maybe<T2>();
      if (x_m.has_value() && y_m.has_value()) return std::pair{*x_m, *y_m};

      renderer::log<renderer::log_type::ERROR>("object_cache",
        std::format("Expected a pair ({}, {}), but got ({}, {}) instead at {}",
          map::type_name<T1>(), map::type_name<T2>(), x.type_name(), y.type_name(), generated.str()
        )
      );
      return std::nullopt;
    }

    renderer::log<renderer::log_type::ERROR>("object_cache",
      std::format("Expected a pair ({}, {}), but got {} instead at {}",
        map::type_name<T1>(), map::type_name<T2>(), type_name(), generated.str()
      )
    );
    return std::nullopt;
  }

  /**
   * @brief Gets the location of the value.
   * @return The location where the value was generated.
   */
  [[nodiscard]] constexpr const loc &pos() const { return generated; }

  value &relocate(const loc &pos) { generated = pos; return *this; }

  value operator*(const value &other) const {
    return with_int_float(other,
      [this](const int x, const int y) { return value{x * y, generated}; },
      [this](const float x, const float y) { return value{x * y, generated}; },
      [this, &other] {
        log_operand_mismatch("*", other);
        return value{0, generated};
      }
    );
  }

  value operator/(const value &other) const {
    return with_int_float(other,
      [this](const int x, const int y) { return value{x / y, generated}; },
      [this](const float x, const float y) { return value{x / y, generated}; },
      [this, &other] {
        log_operand_mismatch("/", other);
        return value{0, generated};
      }
    );
  }

  value operator%(const value &other) const {
    if (is<int>() && other.is<int>()) return value{as<int>() % other.as<int>(), generated};
    log_operand_mismatch("%", other);
    return value{0, generated};
  }

  value operator+(const value &other) const {
    return with_int_float(other,
      [this](const int x, const int y) { return value{x + y, generated}; },
      [this](const float x, const float y) { return value{x + y, generated}; },
      [this, &other] {
        if (is<std::vector<value>>()) {
          const auto &v = as<std::vector<value>>();
          if (other.is<std::vector<value>>()) {
            const auto &ov = other.as<std::vector<value>>();
            std::vector res{v};
            res.insert(res.end(), ov.begin(), ov.end());
            return value{res, generated};
          }

          std::vector res{v};
          res.push_back(other);
          return value{res, generated};
        }

        if (is<std::string>()) {
          return value{as<std::string>() + static_cast<std::string>(other), generated};
        }

        log_operand_mismatch("+", other);
        return value{0, generated};
      }
    );
  }

  value operator-(const value &other) const {
    return with_int_float(other,
      [this](const int x, const int y) { return value{x - y, generated}; },
      [this](const float x, const float y) { return value{x - y, generated}; },
      [this, &other] {
        log_operand_mismatch("-", other);
        return value{0, generated};
      }
    );
  }

  value operator==(const value &other) const {
    return std::visit(
      [this, &other]<typename T>(const T &x) -> value {
        if (other.is<T>()) {
          return value{x == other.as<T>(), generated};
        }
        if constexpr(std::same_as<T, float>) {
          if (other.is<int>())
            return value{x == static_cast<float>(other.as<int>()), generated};
        }
        if constexpr(std::same_as<T, int>) {
          if (other.is<float>())
            return value{static_cast<float>(x) == other.as<float>(), generated};
        }
        log_operand_mismatch("==", other);
        return value{false, generated};
      },
      x
    );
  }

  value operator!=(const value &other) const {
    return std::visit(
      [this, &other]<typename T>(const T &x) -> value {
        if (other.is<T>()) {
          return value{x != other.as<T>(), generated};
        }
        if constexpr(std::same_as<T, float>) {
          if (other.is<int>())
            return value{x != static_cast<float>(other.as<int>()), generated};
        }
        if constexpr(std::same_as<T, int>) {
          if (other.is<float>())
            return value{static_cast<float>(x) != other.as<float>(), generated};
        }
        log_operand_mismatch("!=", other);
        return value{false, generated};
      },
      x
    );
  }

  value operator<(const value &other) const {
    return with_int_float(other,
      [this](const int x, const int y) { return value{x < y, generated}; },
      [this](const float x, const float y) { return value{x < y, generated}; },
      [this, &other] {
        log_operand_mismatch("<", other);
        return value{false, generated};
      }
    );
  }

  value operator>(const value &other) const {
    return with_int_float(other,
      [this](const int x, const int y) { return value{x > y, generated}; },
      [this](const float x, const float y) { return value{x > y, generated}; },
      [this, &other] {
        log_operand_mismatch(">", other);
        return value{false, generated};
      }
    );
  }

  value operator<=(const value &other) const {
    return with_int_float(other,
      [this](const int x, const int y) { return value{x <= y, generated}; },
      [this](const float x, const float y) { return value{x <= y, generated}; },
      [this, &other] {
        log_operand_mismatch("<=", other);
        return value{false, generated};
      }
    );
  }

  value operator>=(const value &other) const {
    return with_int_float(other,
      [this](const int x, const int y) { return value{x >= y, generated}; },
      [this](const float x, const float y) { return value{x >= y, generated}; },
      [this, &other] {
        log_operand_mismatch(">=", other);
        return value{false, generated};
      }
    );
  }

  bool operator!() const {
    // NOT MEANT FOR ACTUAL USE!
    if (is<bool>()) return !as<bool>();
    OPENVTT_UNREACHABLE;
  }

  explicit operator std::string() const {
    if (is<std::string>()) return as<std::string>();
    if (is<bool>()) {
      if (as<bool>()) return "true";
      return "false";
    }
    if (is<int>()) return std::to_string(as<int>());
    if (is<float>()) return std::to_string(as<float>());
    if (is<glm::vec3>()) {
      const auto &v = as<glm::vec3>();
      return std::format("({:.2f}, {:.2f}, {:.2f})", v.x, v.y, v.z);
    }
    if (is<glm::mat4>()) {
      const auto &m = as<glm::mat4>();
      return std::format(
        "mat4[[{}, {}, {}, {}], [{}, {}, {}, {}], [{}, {}, {}, {}], [{}, {}, {}, {}]]",
        m[0].x, m[0].y, m[0].z, m[0].w, m[1].x, m[1].y, m[1].z, m[1].w,
        m[2].x, m[2].y, m[2].z, m[2].w, m[3].x, m[3].y, m[3].z, m[3].w
      );
    }
    if (is<renderer::object_ref>()) return std::format("object#{}", as<renderer::object_ref>().raw());
    if (is<renderer::instanced_object_ref>()) return std::format("instanced_object#{}", as<renderer::instanced_object_ref>().raw());
    if (is<renderer::shader_ref>()) return std::format("shader#{}", as<renderer::shader_ref>().raw());
    if (is<renderer::texture_ref>()) return std::format("texture#{}", as<renderer::texture_ref>().raw());
    if (is<renderer::collider_ref>()) return std::format("collider#{}", as<renderer::collider_ref>().raw());
    if (is<renderer::instanced_collider_ref>()) return std::format("instanced_collider#{}", as<renderer::instanced_collider_ref>().raw());
    if (is<renderer::render_ref>()) return std::format("renderable#{}", as<renderer::render_ref>().raw());
    if (is<renderer::instanced_render_ref>()) return std::format("instanced_renderable#{}", as<renderer::instanced_render_ref>().raw());
    if (is<voxel_corner>()) {
      return voxel_corner_str(as<voxel_corner>());
    }
    if (is<voxel_desc>()) {
      const auto &[c0, c1, c2, c3, c4, c5, c6, c7, c8] = as<voxel_desc>();
      return std::format(
        "voxel[ tl={}, tc={}, tr={}, cl={}, c={}, cr={}, bl={}, bc={}, br={} ]",
        voxel_corner_str(c0), voxel_corner_str(c1), voxel_corner_str(c2),
        voxel_corner_str(c3), voxel_corner_str(c4), voxel_corner_str(c5),
        voxel_corner_str(c6), voxel_corner_str(c7), voxel_corner_str(c8)
      );
    }
    if (is<value_pair>()) {
      return std::format("({}, {})", static_cast<std::string>(as<value_pair>().first()), static_cast<std::string>(as<value_pair>().second()));
    }
    if (is<std::vector<value>>()) {
      const auto &v = as<std::vector<value>>();
      if (v.empty()) return "[]";

      std::stringstream strm;
      strm << "[ " << static_cast<std::string>(v[0]);
      for (size_t i = 1; i < v.size(); ++i) {
        strm << ", " << static_cast<std::string>(v[i]);
      }
      strm << " ]";
      return strm.str();
    }
    if (is<std::monostate>()) return "void";
    OPENVTT_UNREACHABLE;
  }

private:
  static std::string voxel_corner_str(const voxel_corner &vc) {
    const auto &[a, b, c] = vc;
    return std::format("back=({}, {}, {}), front=({}, {}, {}), scale={}", a.x, a.y, a.z, b.x, b.y, b.z, c);
  }

  void log_operand_mismatch(const std::string &op, const value &other) const {
    renderer::log<renderer::log_type::ERROR>("value", "Invalid operands for operator{}: {} (at {}) {} {} (at {})", op, type_name(), generated.str(), op, other.type_name(), other.generated.str());
  }

  using var_t = std::variant<
    bool, int, float, std::string, glm::vec3, glm::mat4,
    renderer::object_ref, renderer::instanced_object_ref, renderer::shader_ref, renderer::texture_ref,
    renderer::collider_ref, renderer::instanced_collider_ref, renderer::render_ref, renderer::instanced_render_ref,
    voxel_corner, voxel_desc,
    value_pair, std::vector<value>, std::monostate
  >;

  template <std::invocable<int, int> F1, std::invocable<float, float> F2, std::invocable<> FE>
  requires(std::same_as<std::invoke_result_t<F1, int, int>, std::invoke_result_t<F2, float, float>> &&
    std::same_as<std::invoke_result_t<F2, float, float>, std::invoke_result_t<FE>>
  )
  auto with_int_float(const value &other, F1 &&f1, F2 && f2, FE &&fe) const -> std::invoke_result_t<F1, int, int> {
    if (is<int>()) {
      if (other.is<int>()) return f1(as<int>(), other.as<int>());
      if (other.is<float>()) return f2(static_cast<float>(as<int>()), other.as<float>());
    }
    if (is<float>()) {
      if (other.is<int>()) return f2(as<float>(), static_cast<float>(other.as<int>()));
      if (other.is<float>()) return f2(as<float>(), other.as<float>());
    }

    return fe();
  }

  var_t x;
  loc generated;
};

inline bool value_pair::operator==(const value_pair &other) const {
  const auto &v1 = first() == other.first();
  const auto &v2 = second() == other.second();
  return v1.as<bool>() && v2.as<bool>();
}


/**
 * @brief A type trait holding default values for each valid value type.
 * @tparam T The valid value type.
 */
template <valid_value T> struct default_value;
template <> struct default_value<bool> { static constexpr bool value = false; };
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

/**
 * @brief A helper variable holding the default value for each valid value type.
 * @tparam T The valid value type.
 */
template <valid_value T>
inline T default_value_v = default_value<T>::value;

/**
 * @brief Forces a value into a certain target type.
 * @tparam T The target type.
 * @return The value as the target type, or the default value for that type.
 *
 * If the value is not of the target type, an error message is logged, and the default value for the target type is
 * returned.
 * Otherwise, the value is returned as the target type.
 */
template<valid_value T>
T value::should_be() const {
  return expecting<T>().value_or(default_value_v<T>);
}

/**
 * @brief A class representing an object cache.
 *
 * The object caches store variables and values that are used in the map script.
 */
class object_cache {
public:
  /**
   * @brief Attempts to look up a variable in the cache (without error message).
   * @param name The name of the variable.
   * @return The value of the variable, or `std::nullopt` if the variable does not exist.
   */
  std::optional<value> lookup_var_maybe(const std::string &name) const {
    const auto it = ctx.find(name);
    return it == ctx.end() ? std::nullopt : std::optional{it->second.first};
  }

  /**
   * @brief Attempts to look up a variable in the cache (with error message).
   * @param name The name of the variable.
   * @param at The location where the variable is used.
   * @return The value of the variable, or `std::nullopt` if the variable does not exist.
   */
  std::optional<value> lookup_var_maybe(const std::string &name, const loc &at) const {
    return with_empty(lookup_var_maybe(name), [&at, &name]() {
      renderer::log<renderer::log_type::ERROR>("object_cache",
        std::format("Variable {} does not exist at {}", name, at.str())
      );
    });
  }

  /**
   * @brief Looks up a variable in the cache.
   * @param name The name of the variable.
   * @param at The location where the variable is used.
   * @return The value of the variable, or a default value if the variable does not exist.
   *
   * If the variable does not exist, an error message is logged.
   */
  value lookup_var(const std::string &name, const loc &at) const {
    return lookup_var_maybe(name, at) || [] { return value{}; };
  }

  /**
   * @brief Looks up a variable in the cache, and type-checks it.
   * @tparam T The type to check for.
   * @param name The name of the variable.
   * @param at The location where the variable is used.
   * @return The value of the variable, or a default value if the variable does not exist or is of the wrong type.
   *
   * This function does not perform integer-to-float promotion.
   */
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

  /**
   * @brief Attempts to assign to a variable in the cache.
   * @tparam T The type of the value to assign.
   * @param name The name of the variable.
   * @param val The value to assign.
   * @param at The location where the assignment is made.
   *
   * If the variable does not exist, it is created (declared).
   * If the variable exists, it is overwritten if the types match.
   * If the variable exists, but the types don't match, an error message is logged.
   */
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

  /**
   * @brief Attempts to assign to a variable in the cache.
   * @param name The name of the variable.
   * @param val The value to assign.
   * @param at The location where the assignment is made.
   *
   * This function is a wrapper around `assign` that forwards the value to the correct overload.
   */
  void assign(const std::string &name, value &&val, const loc &at) {
    val.visit([this, &name, &at]<typename T>(T &&t) { assign(name, t, at); });
  }

private:
  std::unordered_map<std::string, std::pair<value, loc>> ctx;
};
}

#endif //OBJECT_CACHE_HPP
