//
// Created by jay on 12/14/24.
//

#ifndef UTIL_HPP
#define UTIL_HPP

#include <optional>
#include <variant>
#include <any>
#include <string>

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#endif

namespace openvtt {
/**
 * @brief Helper struct to check if a type is an `std::optional`.
 * @tparam T The type to check.
 */
template <typename T> struct is_optional_helper : std::false_type {};
template <typename T> struct is_optional_helper<std::optional<T>> : std::true_type {};
/**
 * @brief Concept to check if a type is an `std::optional`.
 * @tparam T The type to check.
 */
template <typename T>
concept is_optional = is_optional_helper<T>::value;

/**
 * @brief Helper struct to get the type contained in an `std::optional`.
 * @tparam T The type to check.
 *
 * This struct provides a `type` member that is the type contained in the `std::optional`, if and only if `T` is an
 * `std::optional<T2>`.
 */
template <typename T> struct optional_held {};
template <typename T> struct optional_held<std::optional<T>> { using type = T; };
/**
 * @brief Alias for the type contained in an `std::optional`.
 * @tparam T The type to check.
 */
template <is_optional T>
using optional_held_t = typename optional_held<T>::type;

/**
 * @brief Performs mapping over an `std::optional`.
 * @tparam T The type contained in the `std::optional`.
 * @tparam F The function to apply to the contained value (should have signature `(T) -> ?`).
 * @param o The `std::optional` to map over.
 * @param f The function to apply to the contained value.
 * @return The result of applying `f` (wrapped in an `std::optional` again), if `o != std::nullopt`.
 *
 * If `o` holds no value, this function returns `std::nullopt`. <br>
 * If `f(o)` returns a value of type `U`, this function returns `std::optional<U>{f(o)}`. <br>
 * If `f(o)` returns a value of type `std::optional<U>`, this function returns a value of type
 * `std::optional<std::optional<U>>`. To avoid nested `std::optional`s, use the `operator>>` overload.
 */
template <typename T, typename F> requires(std::invocable<F, T>)
std::optional<std::invoke_result_t<F, T>> operator|(const std::optional<T> &o, F &&f) {
  return o.has_value() ? std::optional{f(*o)} : std::nullopt;
}

/**
 * @brief Performs mapping over an `std::optional`.
 * @tparam T The type contained in the `std::optional`.
 * @tparam F The function to apply to the contained value (should have signature `(T) -> ?`).
 * @param o The `std::optional` to map over.
 * @param f The function to apply to the contained value.
 * @return The result of applying `f` (wrapped in an `std::optional` again), if `o != std::nullopt`.
 *
 * If `o` holds no value, this function returns `std::nullopt`. <br>
 * If `f(o)` returns a value of type `U`, this function returns `std::optional<U>{f(o)}`. <br>
 * If `f(o)` returns a value of type `std::optional<U>`, this function returns a value of type
 * `std::optional<std::optional<U>>`. To avoid nested `std::optional`s, use the `operator>>` overload.
 */
template <typename T, typename F> requires(std::invocable<F, T> && std::same_as<std::invoke_result_t<F, T>, void>)
void operator|(const std::optional<T> &o, F &&f) {
  if (o.has_value()) f(*o);
}

/**
 * @brief Performs monadic binding over an `std::optional`.
 * @tparam T The type contained in the `std::optional`.
 * @tparam F The function to apply to the contained value (should have signature `(T) -> std::optional<?>`).
 * @param o The `std::optional` to bind over.
 * @param f The function to apply to the contained value.
 * @return The result of applying `f`, if `o != std::nullopt`.
 *
 * If `o` holds no value, this function returns `std::nullopt`. <br>
 * Otherwise, this function returns `f(o)`. This will not result in nested `std::optional`s. <br>
 * To apply a function that doesn't return an `std::optional`, use the `operator|` overload.
 */
template <typename T, typename F> requires(std::invocable<F, T> && is_optional<std::invoke_result_t<F, T>>)
std::optional<optional_held_t<std::invoke_result_t<F, T>>> operator>>(const std::optional<T> &o, F &&f) {
  if (!o.has_value()) return std::nullopt;
  const auto res = f(*o);
  return res.has_value() ? std::optional{res.value()} : std::nullopt;
}

namespace detail {
template <typename Res, typename T1, typename ... Ts>
constexpr std::optional<Res> any_should_be(const std::any &any) {
  if (auto *ptr = std::any_cast<T1>(any)) return std::optional{Res{*ptr}};
  if constexpr(sizeof...(Ts) != 0) return any_should_be<Res, Ts...>(any);
  return std::nullopt;
}
}

/**
 * @brief Checks whether an `std::any` contains a value of a provided type.
 * @tparam Ts The types to check against.
 * @param any The `std::any` to check.
 * @return A value if possible, otherwise `std::nullopt`.
 *
 * If `any` does not hold any value, returns `std::nullopt`. <br>
 * If `any` holds a value of any type included in `Ts...`, returns a `std::variant` containing that value. <br>
 * Otherwise, returns `std::nullopt`.
 */
template <typename ... Ts>
constexpr std::optional<std::variant<Ts...>> any_should_be(const std::any &any) {
  if (!any.has_value()) return std::nullopt;
  return detail::any_should_be<std::variant<Ts...>, Ts...>(any);
}

/**
 * @brief Executes a function if an `std::optional` holds no value.
 * @tparam T The type contained in the `std::optional`.
 * @tparam F The function to execute (should have signature `() -> T`).
 * @param o The `std::optional` to check.
 * @param f The function to execute.
 * @return The contained value (if any), or the return value of `f()`.
 *
 * If `o` holds a value, this function returns that value. <br>
 * Otherwise, this function returns the result of `f()`.
 *
 * `f` is only invoked if `o` holds no value.
 */
template <typename T, std::invocable<> F>
constexpr T operator||(const std::optional<T> &o, F &&f) {
  return o.has_value() ? *o : f();
}

/**
 * @brief Executes a function if an `std::optional` holds no value.
 * @tparam T The type contained in the `std::optional`.
 * @tparam F The function to execute (should have signature `() -> void`).
 * @param o The `std::optional` to check.
 * @param f The function to execute.
 * @return The original `std::optional`.
 */
template <typename T, std::invocable<> F>
constexpr std::optional<T> with_empty(const std::optional<T> &o, F &&f) {
  if (!o.has_value()) f();
  return o;
}

/**
 * @brief Attempts to de-mangle a mangled C++ name.
 * @param name The mangled name to de-mangle.
 * @return The de-mangled name, or the original name if de-mangling failed.
 *
 * On GCC/Clang, this uses `abi::__cxa_demangle` to de-mangle the name. On other compilers (e.g. MSVC), this function
 * simply returns the original name.
 */
inline std::string demangle(const char *name) {
#if defined(__GNUC__) || defined(__clang__)
  int status = 0;
  char *demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
  if (status == 0) {
    std::string result{demangled};
    free(demangled);
    return result;
  }
  return name;
#else
  return {name};
#endif
}
}

#ifdef OPENVTT_DEBUG
#include "traced_exception.hpp"
#define OPENVTT_UNREACHABLE throw openvtt::traced_exception("Unreachable code should not be reached at " __FILE__ ":" + std::to_string(__LINE__));
#elif defined(OPENVTT_RELEASE)
#define OPENVTT_UNREACHABLE std::unreachable();
#else
#error "No build type specified - define either OPENVTT_DEBUG (for debug mode) or OPENVTT_RELEASE (for release mode)"
#endif

#endif //UTIL_HPP
