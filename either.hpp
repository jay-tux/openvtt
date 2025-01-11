//
// Created by jay on 1/1/25.
//

#ifndef EITHER_HPP
#define EITHER_HPP

#include <variant>
#include <utility>
#include <any>

#include <functional>

namespace openvtt {
/**
 * @brief A tag type representing that a side of an either is empty.
 */
struct either_tag{};

/**
 * @brief A helper type to extract the result of a function call.
 */
template <typename F, typename ... Args> requires(std::invocable<F, Args...>)
using res_t = decltype(std::declval<F>()(std::declval<Args>()...));

/**
 * @brief A class representing a type that can be either of two types.
 * @tparam L The "left" type (often an error type).
 * @tparam R The "right" type (often a success type).
 *
 * This class is a wrapper around std::variant that provides a more functional interface, allowing monadic use (using
 * `map_right` or `operator|` for mapping, and `bind` or `operator>>` for binding).
 */
template <typename L, typename R>
class either {
public:
  /**
   * @brief Constructs an `either` from a left value (by copy).
   * @param l The left value.
   */
  constexpr explicit either(const L &l, int) : x{std::in_place_index<0>, l} {}
  /**
   * @brief Constructs an `either` from a left value (by move).
   * @param l The left value.
   */
  constexpr explicit either(L &&l, int) : x{std::in_place_index<0>, std::move(l)} {}
  /**
   * @brief Constructs an `either` from a right value (by copy).
   * @param r The right value.
   */
  constexpr explicit either(const R &r) : x{std::in_place_index<1>, r} {}
  /**
   * @brief Constructs an `either` from a right value (by move).
   * @param r The right value.
   */
  constexpr explicit either(R &&r) : x{std::in_place_index<1>, std::move(r)} {}

  /**
   * @brief Specifies the `right` type of the `either`.
   * @param other The `either` to copy the left value from.
   *
   * This is mostly used to convert `either<L, either_tag>` to `either<L, R>` automatically and implicitly.
   */
  constexpr either(const either<L, either_tag> &other) requires(!std::same_as<R, either_tag>) : x{std::in_place_index<0>, other.left()} {}
  /**
   * @brief Specifies the `right` type of the `either`.
   * @param other The `either` to move the left value from.
   *
   * This is mostly used to convert `either<L, either_tag>` to `either<L, R>` automatically and implicitly.
   */
  constexpr either(either<L, either_tag> &&other) requires(!std::same_as<R, either_tag>) : x{std::in_place_index<0>, std::move(other.left())} {}
  /**
   * @brief Specifies the `left` type of the `either`.
   * @param other The `either` to copy the right value from.
   *
   * This is mostly used to convert `either<either_tag, R>` to `either<L, R>` automatically and implicitly.
   */
  constexpr either(const either<either_tag, R> &other) requires(!std::same_as<L, either_tag>) : x{std::in_place_index<1>, other.right()} {}
  /**
   * @brief Specifies the `left` type of the `either`.
   * @param other The `either` to move the right value from.
   *
   * This is mostly used to convert `either<either_tag, R>` to `either<L, R>` automatically and implicitly.
   */
  constexpr either(either<either_tag, R> &&other) requires(!std::same_as<L, either_tag>) : x{std::in_place_index<1>, std::move(other.right())} {}

  /**
   * @brief Checks if the `either` is a left value.
   * @return `true` if the `either` is a left value, `false` otherwise.
   */
  [[nodiscard]] constexpr bool is_left() const { return x.index() == 0; }
  /**
   * @brief Checks if the `either` is a right value.
   * @return `true` if the `either` is a right value, `false` otherwise.
   */
  [[nodiscard]] constexpr bool is_right() const { return x.index() == 1; }
  /**
   * @brief Gets the left value of the `either`.
   * @return A reference to the left value.
   */
  constexpr L &left() { return std::get<0>(x); }
  /**
   * @brief Gets the left value of the `either`.
   * @return A const reference to the left value.
   */
  constexpr const L &left() const { return std::get<0>(x); }
  /**
   * @brief Gets the right value of the `either`.
   * @return A reference to the right value.
   */
  constexpr R &right() { return std::get<1>(x); }
  /**
   * @brief Gets the right value of the `either`.
   * @return A const reference to the right value.
   */
  constexpr const R &right() const { return std::get<1>(x); }

private:
  std::variant<L, R> x;
};

/**
 * @brief Helper type to check if a type is an `either`.
 * @tparam T The type to check.
 */
template <typename T> struct _is_either : std::false_type {};
template <typename L, typename R> struct _is_either<either<L, R>> : std::true_type {};
/**
 * @brief Concept to check if a type is an `either`.
 * @tparam T The type to check.
 */
template <typename T> concept is_either = _is_either<T>::value;

/**
 * @brief Helper type to get the left and right types of an `either`.
 * @tparam T The `either` type.
 */
template <typename T> struct either_traits;
template <typename L, typename R> struct either_traits<either<L, R>> {
  using left_t = L; //!< The left type of the `either`.
  using right_t = R; //!< The right type of the `either`.
};

/**
 * @brief Helper function to create an `either` with a left value.
 * @tparam L The left type.
 * @param l The left value.
 * @return The constructed `either`.
 */
template <typename L>
either<L, either_tag> left(const L &l) { return either<L, either_tag>{l, 0}; }
/**
 * @brief Helper function to create an `either` with a right value.
 * @tparam R The right type.
 * @param r The right value.
 * @return The constructed `either`.
 */
template <typename R>
either<either_tag, R> right(const R &r) { return either<either_tag, R>{r}; }

/**
 * @brief If the `either` is a left value, maps it using the function `f`.
 * @tparam L The left type of the `either`.
 * @tparam R The right type of the `either`.
 * @tparam F The function type (should be `(const L &) -> T`).
 * @param e The `either` to map.
 * @param f The function to map with.
 * @return The mapped `either`.
 *
 * If the `either` is a left value, a new `either<T, R>` is constructed with the result of `f(e.left())`.
 * Otherwise, the `either` is copied to be `either<T, R>` instead of `either<L, R>`.
 */
template <typename L, typename R, std::invocable<const L &> F>
constexpr either<res_t<F, const L &>, R> map_left(const either<L, R> &e, F f) {
  if (e.is_left()) return left(f(e.left()));
  return right(e.right());
}

/**
 * @brief If the `either` is a right value, maps it using the function `f`.
 * @tparam L The left type of the `either`.
 * @tparam R The right type of the `either`.
 * @tparam F The function type (should be `(const R &) -> T`).
 * @param e The `either` to map.
 * @param f The function to map with.
 * @return The mapped `either`.
 *
 * If the `either` is a right value, a new `either<L, T>` is constructed with the result of `f(e.right())`.
 * Otherwise, the `either` is copied to be `either<L, T>` instead of `either<L, R>`.
 */
template <typename L, typename R, std::invocable<const R &> F>
constexpr either<L, res_t<F, const R &>> map_right(const either<L, R> &e, F f) {
  if (e.is_right()) return right(f(e.right()));
  return left(e.left());
}

/**
 * @brief If the `either` is a right value, maps it using the function `f`.
 * @tparam L The left type of the `either`.
 * @tparam R The right type of the `either`.
 * @tparam F The function type (should be `(const R &) -> T`).
 * @param e The `either` to map.
 * @param f The function to map with.
 * @return The mapped `either`.
 *
 * If the `either` is a right value, a new `either<L, T>` is constructed with the result of `f(e.right())`.
 * Otherwise, the `either` is copied to be `either<L, T>` instead of `either<L, R>`.
 *
 * This operator is a shorthand for `map_right(e, f)`.
 */
template <typename L, typename R, std::invocable<const R &> F>
constexpr either<L, res_t<F, const R &>> operator|(const either<L, R> &e, F f) {
  return map_right(e, f);
}

/**
 * @brief Binds the `either` to a function.
 * @tparam L The left type of the `either`.
 * @tparam R The right type of the `either`.
 * @tparam F The function type (should be `(const R &) -> either<L, T>`).
 * @param e The `either` to bind.
 * @param f The function to bind with.
 * @return The result of the function call.
 *
 * If the `either` is a left value, a new `either<L, T>` is constructed with the left value.
 * Otherwise, the result of `f(e.right())` is returned.
 */
template <typename L, typename R, std::invocable<const R &> F>
requires(is_either<res_t<F, const R &>> && std::convertible_to<L, typename either_traits<res_t<F, const R &>>::left_t>)
constexpr res_t<F, const R &> bind(const either<L, R> &e, F f) {
  if (e.is_left()) return left(e.left());
  return f(e.right());
}

/**
 * @brief Binds the `either` to a function.
 * @tparam L The left type of the `either`.
 * @tparam R The right type of the `either`.
 * @tparam F The function type (should be `(const R &) -> either<L, T>`).
 * @param e The `either` to bind.
 * @param f The function to bind with.
 * @return The result of the function call.
 *
 * This operator is a shorthand for `bind(e, f)`.
 */
template <typename L, typename R, std::invocable<const R &> F>
requires(is_either<res_t<F, const R &>> && std::convertible_to<L, typename either_traits<res_t<F, const R &>>::left_t>)
constexpr res_t<F, const R &> operator>>(const either<L, R> &e, F f) {
  return bind(e, f);
}

/**
 * @brief Folds the `either` using two functions.
 * @tparam L The left type of the `either`.
 * @tparam R The right type of the `either`.
 * @tparam FL The function type for the left value (should be `(const L &) -> T`).
 * @tparam FR The function type for the right value (should be `(const R &) -> T`).
 * @param e The `either` to fold.
 * @param fl The function to fold with if the `either` is a left value.
 * @param fr The function to fold with if the `either` is a right value.
 * @return The result of the function call.
 *
 * If the `either` is a left value, the result of `fl(e.left())` is returned.
 * Otherwise, the result of `fr(e.right())` is returned.
 */
template <typename L, typename R, std::invocable<const L &> FL, std::invocable<const R &> FR>
requires(std::convertible_to<res_t<FL, const L &>, res_t<FR, const R &>>)
constexpr res_t<FR, const R &> fold(const either<L, R> &e, FL fl, FR fr) {
  if (e.is_left()) return fl(e.left());
  return fr(e.right());
}

/**
 * @brief Helper namespace for internal functions.
 */
namespace _impl {
/**
 * @brief Gets the first `left` value from a set of `either`s.
 * @tparam L The left type of all `either`s.
 * @tparam R The right type of the first `either`.
 * @tparam Rs The right types of the rest of the `either`s.
 * @param e1 The first `either`.
 * @param es The rest of the `either`s.
 * @return The first `left` value.
 *
 * If no `left` value is found, a default-constructed `L` is returned.
 */
template <typename L, typename R, typename ... Rs>
constexpr L first_left(const either<L, R> &e1, const either<L, Rs> &... es) {
  if (e1.is_left()) return e1.left();

  if constexpr(sizeof...(es) == 0) return L{};
  else return first_left(es...);
}
}

/**
 * @brief Merges a set of `either`s into a single `either`.
 * @tparam L The left type of all `either`s.
 * @tparam Rs The right types of all `either`s.
 * @param es The `either`s to merge.
 * @return The merged `either`.
 *
 * If any of the `either`s is a left value, the first `left` value is returned as a `left` value.
 * Otherwise, all `right` values are collected into a tuple and returned as a `right` value.
 */
template <typename L, typename ... Rs>
constexpr either<L, std::tuple<Rs...>> merge(const either<L, Rs> &... es) {
  if ((es.is_right() && ...)) return right(std::tuple{es.right()...});
  return left(_impl::first_left(es...));
}
}

#endif //EITHER_HPP
