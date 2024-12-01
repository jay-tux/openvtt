//
// Created by jay on 11/30/24.
//

#ifndef GL_WRAPPER_HPP
#define GL_WRAPPER_HPP

#ifdef GLTT_USE_STACK_TRACE
#include <ranges>
#include <stacktrace>
#endif
#include <glad/glad.h>
#include "log_view.hpp"

namespace gltt::renderer {
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

inline void on_gl_error(const int gl_error, const char *file, const int line, const char *call) {
  const std::string src = std::format("{}:{}", file, line);
  log<log_type::ERROR>(src, std::format("OpenGL error: {} while calling {}", gl_to_string(gl_error), call));
#ifdef GLTT_USE_STACK_TRACE
  // ReSharper disable once CppTooWideScopeInitStatement
  const auto stack = std::stacktrace::current();
  for (const auto &loc: stack | std::ranges::views::drop(1)) {
    log<log_type::DEBUG>(src, std::format("at: {} ({}:{})", loc.description(), loc.source_file(), loc.source_line()));
  }
#endif
}
}

#define gl(call) do { (call); const auto _ = glGetError(); if(_ != GL_NO_ERROR) gltt::renderer::on_gl_error(_, __FILE__, __LINE__, #call); } while(false)

#endif //GL_WRAPPER_HPP
