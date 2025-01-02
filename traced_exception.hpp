//
// Created by jay on 12/31/24.
//

#ifndef TRACED_EXCEPTION_HPP
#define TRACED_EXCEPTION_HPP

#include <stdexcept>

#ifdef OPENVTT_DEBUG
#include <stacktrace>
#endif

namespace openvtt {
#ifdef OPENVTT_DEBUG
struct traced_exception final : std::runtime_error {
  explicit traced_exception(const std::string &msg) : std::runtime_error(std::format("{}\n{}", msg, std::stacktrace::current())) {
    stack = std::stacktrace::current();
  }

  [[nodiscard]] const std::stacktrace &get_stack() const { return stack; }

  std::stacktrace stack;
};
#elif defined(OPENVTT_RELEASE)
using traced_exception = std::runtime_error;
#else
#error "No build type specified - define either OPENVTT_DEBUG (for debug mode) or OPENVTT_RELEASE (for release mode)"
#endif
}

#endif //TRACED_EXCEPTION_HPP
