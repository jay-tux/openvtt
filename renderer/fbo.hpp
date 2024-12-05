//
// Created by jay on 12/4/24.
//

#ifndef HIGHLIGHT_FBO_HPP
#define HIGHLIGHT_FBO_HPP

#include <utility>

namespace openvtt::renderer {
/**
 * @brief A class wrapping an OpenGL Framebuffer Object.
 *
 * FBO's are used to render to textures, which OpenVTT uses to render highlights over hovered objects.
 */
class fbo {
public:
  /**
   * @brief Construct a new fbo object.
   * @param size The size of the fbo.
   *
   * The size is used to construct a backing texture, and can be resized later. However, resizing the FBO is quite an
   * expensive operation.
   */
  explicit inline fbo(const std::pair<unsigned int, unsigned int> &size) : fbo(size.first, size.second) {}
  /**
   * @brief Construct a new fbo object.
   * @param w The width of the fbo.
   * @param h The height of the fbo.
   *
   * The size is used to construct a backing texture, and can be resized later. However, resizing the FBO is quite an
   * expensive operation.
   */
  fbo(unsigned int w, unsigned int h);
  fbo(const fbo &other) = delete;
  fbo(fbo &&other) noexcept = delete;
  fbo &operator=(const fbo &other) = delete;
  fbo &operator=(fbo &&other) noexcept = delete;

  /**
   * @brief Bind the FBO.
   *
   * Binding the FBO allows you to render to the texture backing the FBO.
   */
  void bind() const;
  /**
   * @brief Clear the FBO.
   *
   * This clears the FBO's backing texture as well. You do not need to bind the FBO to clear it (as this function will
   * take care of that).
   */
  void clear() const;
  /**
   * @brief Unbind the FBO.
   *
   * Unbinding the FBO allows you to render to the screen again, instead of the FBO.
   */
  void unbind() const;
  /**
   * @brief Gets the OpenGL handle for the FBO's backing texture.
   * @return The OpenGL handle for the FBO's backing texture.
   *
   * Prefer using bind_rgb_to instead of this function.
   */
  [[nodiscard]] constexpr unsigned int rgb() const { return rgb_tex; }
  /**
   * @brief Binds the FBO's backing texture to a texture slot.
   *
   * Binding the FBO's texture like this is recommended, and allows shaders to use the texture.
   */
  void bind_rgb_to(unsigned int slot) const;
  /**
   * @brief Verifies if the FBO is valid and complete.
   *
   * This function should only be run after the FBO is modified (i.e. after initialization and resizing).
   */
  [[nodiscard]] bool verify() const;

  /**
   * @brief Resizes the FBO.
   * @param nw The new width of the FBO.
   * @param nh The new height of the FBO.
   *
   * Resizing the FBO is quite expensive, as it requires removing the current backing texture, and creating a new one.
   * This inherently also clears the FBO's backing texture.
   */
  void resize(int nw, int nh);
  /**
   * @brief Resizes the FBO.
   * @param size The new size of the FBO.
   *
   * Resizing the FBO is quite expensive, as it requires removing the current backing texture, and creating a new one.
   * This inherently also clears the FBO's backing texture.
   */
  inline void resize(const std::pair<int, int> &size) { resize(size.first, size.second); }

  /**
   * @brief Run a function with the FBO bound.
   * @tparam F The type of the function to run (should be of type `() -> void`).
   * @param f The function to run.
   *
   * This function will bind the FBO, run the function, and then unbind the FBO. If any FBO was already bound, it will
   * still unbind the FBO after the call (i.e. it will not restore the previous FBO, but rather target the screen again
   * for rendering).
   */
  template <std::invocable F>
  void with_fbo(F &&f) const {
    bind();
    f();
    unbind();
  }

  /**
   * @brief Draw the FBO's backing texture to a Dear IMGUI window.
   * @param name The name of the IMGUI window.
   * @param w The width of the texture in the IMGUI window.
   * @param h The height of the texture in the IMGUI window.
   */
  void draw_texture_imgui(const char *name, int w, int h) const;

  ~fbo();
private:
  unsigned int fbo_id = 0; //!< The FBO ID
  unsigned int rgb_tex = 0; //!< The (RGB) texture ID
  unsigned int w; //!< The width of the FBO
  unsigned int h; //!< The height of the FBO
  mutable int vp_cache[4]{}; //!< The viewport cache, used when binding and unbinding, to restore the viewport state
};
}

#endif //HIGHLIGHT_FBO_HPP
