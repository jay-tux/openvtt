//
// Created by jay on 12/14/24.
//

#include "object_cache.hpp"

using namespace openvtt::map;

value_pair::value_pair() : _first{new value{}}, _second{new value{}} {}
value_pair::value_pair(const value &first, const value &second) : _first{new value{first}}, _second{new value{second}} {}
value_pair::value_pair(value &&first, value &&second) : _first{new value{std::move(first)}}, _second{new value{std::move(second)}} {}

value_pair::value_pair(const value_pair &other) { *this = other; }
value_pair::value_pair(value_pair &&other) noexcept { *this = std::move(other); }

value_pair &value_pair::operator=(const value_pair &other) {
  if (this == &other) return *this;
  delete _first; delete _second;
  _first = new value{other.first()};
  _second = new value{other.second()};
  return *this;
}

value_pair &value_pair::operator=(value_pair &&other) noexcept {
  if (this == &other) return *this;
  std::swap(_first, other._first);
  std::swap(_second, other._second);
  return *this;
}

std::pair<const value &, const value &> value_pair::as_pair() const { return {first(), second()}; }
value &value_pair::first() const { return *_first; }
value &value_pair::second() const { return *_second; }

value_pair::~value_pair() { delete _first; delete _second; }