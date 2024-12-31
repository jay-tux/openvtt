//
// Created by jay on 12/31/24.
//

#ifndef GL_MACROS_HPP
#define GL_MACROS_HPP

#include "log_view.hpp"
#include "glad.h"

namespace openvtt::renderer {
constexpr const char *gl_status_string(const int gl_error) {
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
}

#define RAW_GL_MACRO(call, fmt, ...) do { \
  call; \
  const auto _ = glGetError(); \
  if(_ != GL_NO_ERROR) \
    openvtt::renderer::log<openvtt::renderer::log_type::ERROR>(\
      "OpenGL", \
      std::format("OpenGL error {} while calling " #call " at {}:{} - " fmt, \
        openvtt::renderer::gl_status_string(_), __FILE__, __LINE__, __VA_ARGS__\
      )\
    ); \
  } \
while(false)

#define GL_genVertexArrays(n, arrays) RAW_GL_MACRO((glGenVertexArrays(n, arrays)), "n={}, arrays={}", n, arrays)
#define GL_bindVertexArray(array) RAW_GL_MACRO((glBindVertexArray(array)), "array={}", array)
#define GL_deleteVertexArrays(n, arrays) RAW_GL_MACRO((glDeleteVertexArrays(n, arrays)), "n={}, arrays={}", n, arrays)

#define GL_genBuffers(n, buffers) RAW_GL_MACRO((glGenBuffers(n, buffers)), "n={}, buffers={}", n, buffers)
#define GL_bindBuffer(target, buffer) RAW_GL_MACRO((glBindBuffer(target, buffer)), "target={}, buffer={}", target, buffer)
#define GL_bufferData(target, size, data, usage) RAW_GL_MACRO((glBufferData(target, size, data, usage)), "target={}, size={}, data={}, usage={}", target, size, data, usage)
#define GL_deleteBuffers(n, buffers) RAW_GL_MACRO((glDeleteBuffers(n, buffers)), "n={}, buffers={}", n, buffers)

#define GL_vertexAttribPointer(index, size, type, normalized, stride, pointer) RAW_GL_MACRO((glVertexAttribPointer(index, size, type, normalized, stride, pointer)), "index={}, size={}, type={}, normalized={}, stride={}, pointer={}", index, size, type, normalized, stride, pointer)
#define GL_enableVertexAttribArray(index) RAW_GL_MACRO((glEnableVertexAttribArray(index)), "index={}", index)
#define GL_vertexAttribDivisor(index, divisor) RAW_GL_MACRO((glVertexAttribDivisor(index, divisor)), "index={}, divisor={}", index, divisor)

#define GL_uniform1i(location, v0) RAW_GL_MACRO((glUniform1i(location, v0)), "location={}, v0={}", location, v0)
#define GL_uniform1ui(location, v0) RAW_GL_MACRO((glUniform1ui(location, v0)), "location={}, v0={}", location, v0)
#define GL_uniform1f(location, v0) RAW_GL_MACRO((glUniform1f(location, v0)), "location={}, v0={}", location, v0)
#define GL_uniform2fv(location, count, value) RAW_GL_MACRO((glUniform2fv(location, count, value)), "location={}, count={}, value={}", location, count, value)
#define GL_uniform3fv(location, count, value) RAW_GL_MACRO((glUniform3fv(location, count, value)), "location={}, count={}, value={}", location, count, value)
#define GL_uniform4fv(location, count, value) RAW_GL_MACRO((glUniform4fv(location, count, value)), "location={}, count={}, value={}", location, count, value)
#define GL_uniformMatrix3fv(location, count, transpose, value) RAW_GL_MACRO((glUniformMatrix3fv(location, count, transpose, value)), "location={}, count={}, transpose={}, value={}", location, count, transpose, value)
#define GL_uniformMatrix4fv(location, count, transpose, value) RAW_GL_MACRO((glUniformMatrix4fv(location, count, transpose, value)), "location={}, count={}, transpose={}, value={}", location, count, transpose, value)

#define GL_clear(mask) RAW_GL_MACRO((glClear(mask)), "mask={}", mask)
#define GL_polygonMode(face, mode) RAW_GL_MACRO((glPolygonMode(face, mode)), "face={}, mode={}", face, mode)
#define GL_viewport(x, y, width, height) RAW_GL_MACRO((glViewport(x, y, width, height)), "x={}, y={}, width={}, height={}", x, y, width, height)

#define GL_drawElements(mode, count, type, indices) RAW_GL_MACRO((glDrawElements(mode, count, type, indices)), "mode={}, count={}, type={}, indices={}", mode, count, type, indices)
#define GL_drawElementsInstanced(mode, count, type, indices, primcount) RAW_GL_MACRO((glDrawElementsInstanced(mode, count, type, indices, primcount)), "mode={}, count={}, type={}, indices={}, primcount={}", mode, count, type, indices, primcount)

#define GL_getIntegerv(pname, data) RAW_GL_MACRO((glGetIntegerv(pname, data)), "pname={}, data={}", pname, data)

#define GL_genTextures(n, textures) RAW_GL_MACRO((glGenTextures(n, textures)), "n={}, textures={}", n, textures)
#define GL_bindTexture(target, texture) RAW_GL_MACRO((glBindTexture(target, texture)), "target={}, texture={}", target, texture)
#define GL_texImage2D(target, level, internalformat, width, height, border, format, type, pixels) RAW_GL_MACRO((glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels)), "target={}, level={}, internalformat={}, width={}, height={}, border={}, format={}, type={}, pixels={}", target, level, internalformat, width, height, border, format, type, pixels)
#define GL_texParameteri(target, pname, param) RAW_GL_MACRO((glTexParameteri(target, pname, param)), "target={}, pname={}, param={}", target, pname, param)
#define GL_generateMipmap(target) RAW_GL_MACRO((glGenerateMipmap(target)), "target={}", target)
#define GL_activeTexture(texture) RAW_GL_MACRO((glActiveTexture(texture)), "texture={}", texture)
#define GL_deleteTextures(n, textures) RAW_GL_MACRO((glDeleteTextures(n, textures)), "n={}, textures={}", n, textures)

#define GL_genFramebuffers(n, framebuffers) RAW_GL_MACRO((glGenFramebuffers(n, framebuffers)), "n={}, framebuffers={}", n, framebuffers)
#define GL_bindFramebuffer(target, framebuffer) RAW_GL_MACRO((glBindFramebuffer(target, framebuffer)), "target={}, framebuffer={}", target, framebuffer)
#define GL_framebufferTexture2D(target, attachment, textarget, texture, level) RAW_GL_MACRO((glFramebufferTexture2D(target, attachment, textarget, texture, level)), "target={}, attachment={}, textarget={}, texture={}, level={}", target, attachment, textarget, texture, level)
#define GL_deleteFramebuffers(n, framebuffers) RAW_GL_MACRO((glDeleteFramebuffers(n, framebuffers)), "n={}, framebuffers={}", n, framebuffers)

#define GL_shaderSource(shader, count, string, length) RAW_GL_MACRO((glShaderSource(shader, count, string, length)), "shader={}, count={}, string={}, length={}", shader, count, string, length)
#define GL_compileShader(shader) RAW_GL_MACRO((glCompileShader(shader)), "shader={}", shader)
#define GL_getShaderiv(shader, pname, params) RAW_GL_MACRO((glGetShaderiv(shader, pname, params)), "shader={}, pname={}, params={}", shader, pname, params)
#define GL_getShaderInfoLog(shader, maxLength, length, infoLog) RAW_GL_MACRO((glGetShaderInfoLog(shader, maxLength, length, infoLog)), "shader={}, maxLength={}, length={}, infoLog={}", shader, maxLength, length, infoLog)
#define GL_attachShader(program, shader) RAW_GL_MACRO((glAttachShader(program, shader)), "program={}, shader={}", program, shader)
#define GL_linkProgram(program) RAW_GL_MACRO((glLinkProgram(program)), "program={}", program)
#define GL_getProgramiv(program, pname, params) RAW_GL_MACRO((glGetProgramiv(program, pname, params)), "program={}, pname={}, params={}", program, pname, params)
#define GL_getProgramInfoLog(program, maxLength, length, infoLog) RAW_GL_MACRO((glGetProgramInfoLog(program, maxLength, length, infoLog)), "program={}, maxLength={}, length={}, infoLog={}", program, maxLength, length, infoLog)
#define GL_useProgram(program) RAW_GL_MACRO((glUseProgram(program)), "program={}", program)
#define GL_deleteShader(shader) RAW_GL_MACRO((glDeleteShader(shader)), "shader={}", shader)
#define GL_deleteProgram(program) RAW_GL_MACRO((glDeleteProgram(program)), "program={}", program)

#endif //GL_MACROS_HPP
