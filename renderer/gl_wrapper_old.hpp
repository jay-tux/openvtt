//
// Created by jay on 11/30/24.
//

#ifndef GL_WRAPPER_HPP
#define GL_WRAPPER_HPP

#ifdef OPENVTT_USE_STACK_TRACE
#include <ranges>
#include <stacktrace>
#endif
#include "glad.h"
#include "log_view.hpp"
#include "gl_macros.hpp"

namespace openvtt::renderer {
/**
 * @brief Converts an OpenGL error code to a string.
 *
 * This function takes an OpenGL error code and returns a string representation of the error.
 *
 * @param gl_error The OpenGL error code.
 * @return A string representation of the OpenGL error code.
 */
constexpr const char *gl_to_string(const int gl_error) {
  switch (gl_error) {
    case GL_NO_ERROR: return "GL_NO_ERROR";
    case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
    case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
    default: return "(unknown error)";
  }
}
/**
 * @brief Handles OpenGL errors by logging them.
 *
 * This function is called when an OpenGL error occurs. It logs the error details, including the file and line number
 * where the error occurred, and the OpenGL function call that caused the error. If stack trace support is enabled,
 * it also logs the stack trace.
 *
 * @param gl_error The OpenGL error code.
 * @param file The name of the file where the error occurred.
 * @param line The line number where the error occurred.
 * @param call The OpenGL function call that caused the error.
 */
inline void on_gl_error(const int gl_error, const char *file, const int line, const char *call) {
  const std::string src = std::format("{}:{}", file, line);
  log<log_type::ERROR>("OpenGL", std::format("{}: OpenGL error: {} while calling {}", src, gl_to_string(gl_error), call));
#ifdef OPENVTT_USE_STACK_TRACE
  // ReSharper disable once CppTooWideScopeInitStatement
  const auto stack = std::stacktrace::current();
  for (const auto &loc: stack | std::ranges::views::drop(1)) {
    log<log_type::DEBUG>("OpenGL", std::format("at: {} ({}:{})", loc.description(), loc.source_file(), loc.source_line()));
  }
#endif
}
}

#define gl(call) do { static_assert(0, "REPLACE THIS"); call; const auto _ = glGetError(); if(_ != GL_NO_ERROR) openvtt::renderer::on_gl_error(_, __FILE__, __LINE__, #call); } while(false)

#endif //GL_WRAPPER_HPP
