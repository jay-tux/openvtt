//
// Created by jay on 11/30/24.
//

#ifndef SHADER_HPP
#define SHADER_HPP

#include <string>
#include <glm/glm.hpp>

namespace openvtt::renderer {
/**
 * A class representing an OpenGL shader.
 */
class shader {
public:
  /**
   * Creates a shader from the given vertex and fragment shader source code.
   * @param vs The vertex shader source code.
   * @param fs The fragment shader source code.
   */
  shader(const std::string &vs, const std::string &fs);

  shader(const shader &other) = delete;
  constexpr shader(shader &&other) noexcept {
    std::swap(program, other.program);
  }
  shader &operator=(const shader &other) = delete;
  shader &operator=(shader &&other) = delete;

  /**
   * Creates a shader from the given vertex and fragment shader source code.
   * @param vsf The path to the vertex shader file.
   * @param fsf The path to the fragment shader file.
   * @return The shader.
   *
   * The shader files are resolved using @ref asset_path.
   */
  static shader from_assets(const std::string &vsf, const std::string &fsf);

  /**
   * @brief Returns the location of the uniform with the given name.
   * @param name The name of the uniform.
   * @return The location of the uniform.
   */
  [[nodiscard]] unsigned int loc_for(const std::string &name) const;

  /**
   * @brief Sets the uniform with the given location to the given value.
   * @param loc The location of the uniform.
   * @param b The boolean value to set the uniform to.
   */
  void set_bool(unsigned int loc, bool b) const;
  /**
   * @brief Sets the uniform with the given location to the given value.
   * @param loc The location of the uniform.
   * @param i The integer value to set the uniform to.
   *
   * This function can also be used to bind textures to texture units (samplers).
   */
  void set_int(unsigned int loc, int i) const;
  /**
   * @brief Sets the uniform with the given location to the given value.
   * @param loc The location of the uniform.
   * @param f The float value to set the uniform to.
   */
  void set_float(unsigned int loc, float f) const;
  /**
   * @brief Sets the uniform with the given location to the given value.
   * @param loc The location of the uniform.
   * @param v The vec2 value to set the uniform to.
   */
  void set_vec2(unsigned int loc, const glm::vec2 &v) const;
  /**
   * @brief Sets the uniform with the given location to the given value.
   * @param loc The location of the uniform.
   * @param v The vec3 value to set the uniform to.
   */
  void set_vec3(unsigned int loc, const glm::vec3 &v) const;
  /**
   * @brief Sets the uniform with the given location to the given value.
   * @param loc The location of the uniform.
   * @param v The vec4 value to set the uniform to.
   */
  void set_vec4(unsigned int loc, const glm::vec4 &v) const;
  /**
   * @brief Sets the uniform with the given location to the given value.
   * @param loc The location of the uniform.
   * @param m The mat2 value to set the uniform to.
   */
  void set_mat3(unsigned int loc, const glm::mat3 &m) const;
    /**
     * @brief Sets the uniform with the given location to the given value.
     * @param loc The location of the uniform.
     * @param m The mat4 value to set the uniform to.
     */
  void set_mat4(unsigned int loc, const glm::mat4 &m) const;

  /**
   * @brief Activates the shader.
   */
  void activate() const;

  ~shader();
private:
  unsigned int program = 0; ///< The OpenGL program ID.
};
}

#endif //SHADER_HPP
