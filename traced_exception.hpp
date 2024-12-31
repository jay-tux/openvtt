//
// Created by jay on 12/31/24.
//

#ifndef TRACED_EXCEPTION_HPP
#define TRACED_EXCEPTION_HPP

#include <stacktrace>
#include <stdexcept>

namespace openvtt {
struct traced_exception final : std::runtime_error {
  explicit traced_exception(const std::string &msg) : std::runtime_error(std::format("{}\n{}", msg, std::stacktrace::current())) {
    stack = std::stacktrace::current();
  }

  [[nodiscard]] const std::stacktrace &get_stack() const { return stack; }

  std::stacktrace stack;
};
}

#endif //TRACED_EXCEPTION_HPP
