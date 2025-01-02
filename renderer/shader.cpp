//
// Created by jay on 11/30/24.
//

#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

#include "gl_wrapper.hpp"
#include "window.hpp"
#include "log_view.hpp"
#include "filesys.hpp"
#include "shader.hpp"

using namespace openvtt::renderer;

shader::shader(const std::string &vs, const std::string &fs) {
  window::get(); // force initialized

  const auto v = glCreateShader(GL_VERTEX_SHADER);
  const auto *str = vs.c_str();
  GL_shaderSource(v, 1, &str, nullptr);
  GL_compileShader(v);
  int ok;
  GL_getShaderiv(v, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    int len;
    GL_getShaderiv(v, GL_INFO_LOG_LENGTH, &len);
    auto *data = new char[len];
    GL_getShaderInfoLog(v, len, &len, data);
    log<log_type::ERROR>("shader", std::format("Failed to compile vertex shader: {}", data));
    delete[] data;
  }

  const auto f = glCreateShader(GL_FRAGMENT_SHADER);
  str = fs.c_str();
  GL_shaderSource(f, 1, &str, nullptr);
  GL_compileShader(f);
  GL_getShaderiv(f, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    int len;
    GL_getShaderiv(f, GL_INFO_LOG_LENGTH, &len);
    auto *data = new char[len];
    GL_getShaderInfoLog(f, len, &len, data);
    log<log_type::ERROR>("shader", std::format("Failed to compile fragment shader: {}", data));
    delete[] data;
  }

  program = glCreateProgram();
  GL_attachShader(program, v);
  GL_attachShader(program, f);
  GL_linkProgram(program);
  GL_getProgramiv(program, GL_LINK_STATUS, &ok);
  if (!ok) {
    int len;
    GL_getProgramiv(program, GL_INFO_LOG_LENGTH, &len);
    auto *data = new char[len];
    GL_getProgramInfoLog(program, len, &len, data);
    log<log_type::ERROR>("shader", std::format("Failed to link shader: {}", data));
    delete[] data;
  }

  GL_deleteShader(v);
  GL_deleteShader(f);
}

shader shader::load_from(const std::string &vsf, const std::string &fsf) {
  auto vs_path = asset_path<asset_type::VERT_SHADER>(vsf);
  log<log_type::DEBUG>("shader", std::format("Loading vertex shader from '{}'", vs_path));

  std::ifstream v_strm(vs_path);
  std::string v_src;
  if (!v_strm.is_open()) {
    log<log_type::ERROR>("shader", std::format("Failed to open '{}'", vs_path));
  }
  else {
    std::stringstream strm;
    strm << v_strm.rdbuf();
    v_src = strm.str();
  }

  auto fs_path = asset_path<asset_type::FRAG_SHADER>(fsf);
  log<log_type::DEBUG>("shader", std::format("loading fragment shader from '{}'", fs_path));

  std::ifstream f_strm(fs_path);
  std::string f_src;
  if (!f_strm.is_open()) {
    log<log_type::ERROR>("shader", std::format("Failed to open '{}'", fs_path));
  }
  else {
    std::stringstream strm;
    strm << f_strm.rdbuf();
    f_src = strm.str();
  }

  return {v_src, f_src};
}

void shader::set_bool(const unsigned int loc, const bool b) const {
  activate(); GL_uniform1i(loc, b);
}
void shader::set_int(const unsigned int loc, const int i) const {
  activate(); GL_uniform1i(loc, i);
}
void shader::set_uint(const unsigned int loc, const unsigned int i) const {
  activate(); GL_uniform1ui(loc, i);
}
void shader::set_float(const unsigned int loc, const float f) const {
  activate(); GL_uniform1f(loc, f);
}
void shader::set_vec2(const unsigned int loc, const glm::vec2 &v) const {
  activate(); GL_uniform2fv(loc, 1, glm::value_ptr(v));
}
void shader::set_vec3(const unsigned int loc, const glm::vec3 &v) const {
  activate(); GL_uniform3fv(loc, 1, glm::value_ptr(v));
}
void shader::set_vec4(const unsigned int loc, const glm::vec4 &v) const {
  activate(); GL_uniform4fv(loc, 1, glm::value_ptr(v));
}
void shader::set_mat3(const unsigned int loc, const glm::mat3 &m) const {
  activate(); GL_uniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}
void shader::set_mat4(const unsigned int loc, const glm::mat4 &m) const {
  activate(); GL_uniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}
void shader::set_mat4x3(const unsigned int loc, const glm::mat4x3 &m) const {
  activate(); GL_uniformMatrix4x3fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}

void shader::activate() const {
  GL_useProgram(program);
}

shader::~shader() {
  GL_deleteProgram(program);
}

unsigned int shader::loc_for(const std::string &name) const {
  return glGetUniformLocation(program, name.c_str());
}
