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
struct either_tag{};

template <typename F, typename ... Args> requires(std::invocable<F, Args...>)
using res_t = decltype(std::declval<F>()(std::declval<Args>()...));

template <typename L, typename R>
class either {
public:
  constexpr explicit either(const L &l, int) : x{std::in_place_index<0>, l} {}
  constexpr explicit either(L &&l, int) : x{std::in_place_index<0>, std::move(l)} {}
  constexpr explicit either(const R &r) : x{std::in_place_index<1>, r} {}
  constexpr explicit either(R &&r) : x{std::in_place_index<1>, std::move(r)} {}

  constexpr either(const either<L, either_tag> &other) requires(!std::same_as<R, either_tag>) : x{std::in_place_index<0>, other.left()} {}
  constexpr either(either<L, either_tag> &&other) requires(!std::same_as<R, either_tag>) : x{std::in_place_index<0>, std::move(other.left())} {}
  constexpr either(const either<either_tag, R> &other) requires(!std::same_as<L, either_tag>) : x{std::in_place_index<1>, other.right()} {}
  constexpr either(either<either_tag, R> &&other) requires(!std::same_as<L, either_tag>) : x{std::in_place_index<1>, std::move(other.right())} {}

  [[nodiscard]] constexpr bool is_left() const { return x.index() == 0; }
  [[nodiscard]] constexpr bool is_right() const { return x.index() == 1; }
  constexpr L &left() { return std::get<0>(x); }
  constexpr const L &left() const { return std::get<0>(x); }
  constexpr R &right() { return std::get<1>(x); }
  constexpr const R &right() const { return std::get<1>(x); }

private:
  std::variant<L, R> x;
};

template <typename T> struct _is_either : std::false_type {};
template <typename L, typename R> struct _is_either<either<L, R>> : std::true_type {};
template <typename T> concept is_either = _is_either<T>::value;

template <typename T> struct either_traits;
template <typename L, typename R> struct either_traits<either<L, R>> {
  using left_t = L;
  using right_t = R;
};

template <typename L>
either<L, either_tag> left(const L &l) { return either<L, either_tag>{l, 0}; }
template <typename R>
either<either_tag, R> right(const R &r) { return either<either_tag, R>{r}; }

template <typename L, typename R, std::invocable<const L &> F>
constexpr either<res_t<F, const L &>, R> map_left(const either<L, R> &e, F f) {
  if (e.is_left()) return left(f(e.left()));
  return right(e.right());
}

template <typename L, typename R, std::invocable<const R &> F>
constexpr either<L, res_t<F, const R &>> map_right(const either<L, R> &e, F f) {
  if (e.is_right()) return right(f(e.right()));
  return left(e.left());
}

template <typename L, typename R, std::invocable<const R &> F>
constexpr either<L, res_t<F, const R &>> operator|(const either<L, R> &e, F f) {
  return map_right(e, f);
}

template <typename L, typename R, std::invocable<const R &> F>
requires(is_either<res_t<F, const R &>> && std::convertible_to<L, typename either_traits<res_t<F, const R &>>::left_t>)
constexpr res_t<F, const R &> operator>>(const either<L, R> &e, F f) {
  if (e.is_left()) return left(e.left());
  return f(e.right());
}

template <typename L, typename R, std::invocable<const L &> FL, std::invocable<const R &> FR>
requires(std::convertible_to<res_t<FL, const L &>, res_t<FR, const R &>>)
constexpr res_t<FR, const R &> fold(const either<L, R> &e, FL fl, FR fr) {
  if (e.is_left()) return fl(e.left());
  return fr(e.right());
}
namespace _impl {
template <typename L, typename R, typename ... Rs>
constexpr L first_left(const either<L, R> &e1, const either<L, Rs> &... es) {
  if (e1.is_left()) return e1.left();

  if constexpr(sizeof...(es) == 0) return L{};
  else return first_left(es...);
}
}

template <typename L, typename ... Rs>
constexpr either<L, std::tuple<Rs...>> merge(const either<L, Rs> &... es) {
  if ((es.is_right() && ...)) return right(std::tuple{es.right()...});
  return left(_impl::first_left(es...));
}
}

#endif //EITHER_HPP
