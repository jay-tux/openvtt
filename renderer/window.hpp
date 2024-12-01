//
// Created by jay on 11/29/24.
//

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <concepts>
#include <imgui.h>
#include <GLFW/glfw3.h>

namespace gltt::renderer {
class window {
public:
  window(const window &) = delete;
  window(window &&) = delete;

  window &operator=(const window &) = delete;
  window &operator=(window &&) = delete;

  static window &get();

  [[nodiscard]] constexpr bool should_close() const { return closing; }
  void request_close() { closing = true; }
  void cancel_close() { closing = false; }

  [[nodiscard]] constexpr ImGuiIO &io_data() const { return *io; }

  template <std::invocable F>
  void with_jb_mono(F &&f) const { ImGui::PushFont(jb_mono_font); f(); ImGui::PopFont(); }
  template <std::invocable F>
  void with_nerd_icons(F &&f) const { ImGui::PushFont(nerd_icons_font); f(); ImGui::PopFont(); }

  [[nodiscard]] float get_time() const;
  [[nodiscard]] float aspect_ratio() const;
  [[nodiscard]] float delta_time_ms() const;
  [[nodiscard]] float delta_time_s() const;

  bool frame_pre();
  void frame_post();

  ~window();

private:
  window();

  GLFWwindow *win;
  ImGuiIO *io;
  ImFont *jb_mono_font;
  ImFont *nerd_icons_font;
  bool closing = false;
};
}

#endif //WINDOW_HPP
