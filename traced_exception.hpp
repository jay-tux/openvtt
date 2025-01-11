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
/**
 * @brief A struct that extends std::runtime_error with a stacktrace.
 */
struct traced_exception final : std::runtime_error {
  /**
   * @brief Construct a new traced exception object
   * @param msg The message to display
   */
  explicit traced_exception(const std::string &msg) : std::runtime_error(std::format("{}\n{}", msg, std::stacktrace::current())) {
    stack = std::stacktrace::current();
  }

  /**
   * @brief Gets the stacktrace
   * @return The stacktrace
   */
  [[nodiscard]] const std::stacktrace &get_stack() const { return stack; }

  std::stacktrace stack; //!< The stack trace.
};
#elif defined(OPENVTT_RELEASE)
using traced_exception = std::runtime_error;
#else
#error "No build type specified - define either OPENVTT_DEBUG (for debug mode) or OPENVTT_RELEASE (for release mode)"
#endif
}

#endif //TRACED_EXCEPTION_HPP
