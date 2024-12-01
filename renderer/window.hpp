//
// Created by jay on 11/29/24.
//

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <concepts>
#include <imgui.h>
#include <GLFW/glfw3.h>

/**
* @brief The renderer namespace contains all the rendering code for the engine.
*/
namespace openvtt::renderer {
/**
 * @brief The window class is a singleton that manages the window and input for the engine.
 *
 * This class takes care of both the 3D (OpenGL) scene and the 2D (Dear IMGUI) UI.
 */
class window {
public:
  window(const window &) = delete;
  window(window &&) = delete;

  window &operator=(const window &) = delete;
  window &operator=(window &&) = delete;

  /**
   * @brief Get the singleton instance of the window class.
   * @return The window instance.
   */
  static window &get();

  /**
   * @brief Checks if the window should close.
   * @return True if the window should close, false otherwise.
   */
  [[nodiscard]] constexpr bool should_close() const { return closing; }
  /**
   * @brief Requests the window to close.
   */
  void request_close() { closing = true; }
  /**
   * @brief Cancels the request to close the window.
   */
  void cancel_close() { closing = false; }

  /**
   * @brief Get ImGui IO data.
   * @return A reference to the ImGui IO data.
   */
  [[nodiscard]] constexpr ImGuiIO &io_data() const { return *io; }

  /**
   * @brief Renders a piece of the UI using the JetBrains Mono font.
   * @tparam F The type of the function used to render (`() -> void`).
   * @param f The function used to render.
   *
   * Practically, this uses `ImGui::PushFont` and `ImGui::PopFont` to provide a safe "scope" for rendering.
   */
  template <std::invocable F>
  void with_jb_mono(F &&f) const { ImGui::PushFont(jb_mono_font); f(); ImGui::PopFont(); }
  /**
   * @brief Renders a piece of the UI using the Nerd Icons font.
   * @tparam F The type of the function used to render (`() -> void`).
   * @param f The function used to render.
   *
   * Practically, this uses `ImGui::PushFont` and `ImGui::PopFont` to provide a safe "scope" for rendering.
   */
  template <std::invocable F>
  void with_nerd_icons(F &&f) const { ImGui::PushFont(nerd_icons_font); f(); ImGui::PopFont(); }

  /**
   * @brief Get the global rendering time (using ImGui).
   * @return The global time.
   */
  [[nodiscard]] float get_time() const;
  /**
   * @brief Get the aspect ratio of the window.
   * @return The aspect ratio.
   */
  [[nodiscard]] float aspect_ratio() const;
  /**
   * @brief Get the time since the previous frame (ms).
   * @return Delta time in milliseconds.
   */
  [[nodiscard]] float delta_time_ms() const;
  /**
   * @brief Get the time since the previous frame (s).
   * @return Delta time in seconds.
   */
  [[nodiscard]] float delta_time_s() const;

  /**
   * @brief Perform pre-frame operations.
   * @return True if the frame should be rendered, false otherwise.
   *
   * Pre-frame operations mostly consist of polling events and Dear IMGUI setup.
   *
   * If this function returns `false`, the window is iconified. This can be used to skip rendering.
   */
  bool frame_pre();
  /**
   * @brief Perform post-frame operations.
   *
   * Post-frame operations mostly consist of rendering the Dear IMGUI UI.
   */
  void frame_post();

  ~window();

private:
  window();

  GLFWwindow *win; //!< The GLFW window.
  ImGuiIO *io; //!< The ImGui IO data.
  ImFont *jb_mono_font; //!< The JetBrains Mono font.
  ImFont *nerd_icons_font; //!< The Nerd Icons font.
  bool closing = false; //!< Whether the window should close.
};
}

#endif //WINDOW_HPP
