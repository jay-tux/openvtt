//
// Created by jay on 11/29/24.
//

#ifndef LOG_VIEW_HPP
#define LOG_VIEW_HPP

#include <vector>
#include <string>
#include <iostream>
#include <format>

namespace gltt::renderer {
enum class log_type {
  DEBUG, INFO, WARNING, ERROR
};

struct log_message {
  std::string source;
  std::string message;
  log_type type = log_type::INFO;
};

class log_view {
public:
  inline log_view &operator<<(const log_message &message) {
    log(message);
    return *this;
  }

  static inline void clear() {
    recent_logs.clear();
  }

  static inline void log(const log_message &message) {
    recent_logs.push_back(message);
    std::cout << std::format("[{:10s}]: {}\n", message.source, message.message);
  }

  static void render();
private:
  static inline std::vector<log_message> recent_logs{};
};

static inline log_view logger;

template <log_type type>
inline void log(const std::string &source, const std::string &message) {
  logger << log_message{source, message, type};
}
}

#endif //LOG_VIEW_HPP
