//
// Created by jay on 11/29/24.
//

#ifndef LOG_VIEW_HPP
#define LOG_VIEW_HPP

#include <vector>
#include <string>
#include <iostream>
#include <format>

template <typename T>
requires(!std::same_as<T, void> && !std::same_as<T, const void> && !std::same_as<T, char> && !std::same_as<T, const char>)
struct std::formatter<T *, char> {
  std::formatter<const void *> base_const{};

  template <typename PC> constexpr auto parse(PC &ctx) {
    return base_const.parse(ctx);
  }
  template <typename FC> constexpr typename FC::iterator format(T *ptr, FC &ctx) const {
    return base_const.format(static_cast<const void *>(ptr), ctx);
  }
};

template <typename T, size_t s>
requires(!std::same_as<T, char> && !std::same_as<T, const char>)
struct std::formatter<T[s], char> {
  std::formatter<const T *> base_const{};

  template <typename PC> constexpr auto parse(PC &ctx) {
    return base_const.parse(ctx);
  }
  template <typename FC> constexpr typename FC::iterator format(const T (&arr)[s], FC &ctx) const {
    return base_const.format(&arr[0], ctx);
  }
};

namespace openvtt::renderer {
/**
 * @brief Enum class representing different types of log messages.
 */
enum class log_type {
  DEBUG,   /**< Debug level log message */
  INFO,    /**< Informational log message */
  WARNING, /**< Warning level log message */
  ERROR    /**< Error level log message */
};

/**
 * @brief Struct representing a log message.
 */
struct log_message {
  std::string source; //!< Source of the log message
  std::string message; //!< Message to be logged
  log_type type = log_type::INFO; //!< Type of the message
};

/**
 * @brief Class to handle logging and rendering log messages.
 */
class log_view {
public:
  /**
   * @brief Overloaded operator to log a message.
   * @param message The message to be logged.
   * @return Reference to the log_view object.
   */
  inline log_view &operator<<(const log_message &message) {
    log(message);
    return *this;
  }

  /**
   * @brief Clears the log, removing all messages.
   */
  static inline void clear() {
    recent_logs.clear();
  }

  /**
   * @brief Logs a message.
   * @param message The message to be logged.
   *
   * This message is both added to the (internal) list of logs to be rendered, and printed to the console.
   */
  static inline void log(const log_message &message) {
    recent_logs.push_back(message);
    std::cout << std::format("[{:10.10s}]: {}\n", message.source, message.message);
  }

  /**
   * @brief Renders the log messages.
   */
  static void render();
private:
  static inline std::vector<log_message> recent_logs{};
};

/**
 * @brief Global log_view object to be used for logging.
 */
static inline log_view logger;

/**
 * @brief Logs a message with the given source and message.
 * @tparam type The type of the log message.
 * @param source The source of the log message.
 * @param message The message to be logged.
 */
template <log_type type>
inline void log(const std::string &source, const std::string &message) {
  logger << log_message{source, message, type};
}

/**
 * @brief Logs a formatted message with the given source.
 * @tparam type The type of the log message.
 * @tparam Args The types of the arguments to be formatted.
 * @param source The source of the log message.
 * @param fmt The format string.
 * @param args The arguments to be formatted.
 */
template <log_type type, typename ... Args> requires(sizeof...(Args) > 0)
inline void log(const std::string &source, const std::format_string<Args...> &fmt, Args &&... args) {
  logger << log_message{source, std::format(fmt, std::forward<Args &&>(args)...), type};
}
}

#endif //LOG_VIEW_HPP
