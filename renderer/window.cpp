//
// Created by jay on 11/29/24.
//

// ReSharper disable CppMemberFunctionMayBeStatic

#include "gl_wrapper.hpp"

#include <stdexcept>
#include <imgui.h>
#include <bindings/imgui_impl_glfw.h>
#include <bindings/imgui_impl_opengl3.h>

#include "window.hpp"
#include "filesys.hpp"
#include "log_view.hpp"

using namespace openvtt::renderer;

window &window::get() {
  static window w;
  return w;
}

window::window() {
  log<log_type::DEBUG>("window", "Initializing GLFW/GLAD/ImGUI");
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW3");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  // Surprisingly fun + useful to debug B/W textures
  // glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GL_TRUE);

  win = glfwCreateWindow(1920, 1080, "OpenVTT", nullptr, nullptr);
  if (!win) {
    glfwTerminate();
    throw std::runtime_error("Failed to open GLFW3 window");
  }
  glfwMakeContextCurrent(win);
  glfwSwapInterval(1); // VSync

  if (gladLoadGL(glfwGetProcAddress) == 0) {
    glfwTerminate();
    throw std::runtime_error("Failed to initialize GLAD");
  }

  glEnable(GL_DEPTH_TEST);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  this->io = &io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();

  const std::string jb_path = asset_path<asset_type::FONT>("JetBrains-Mono");
  log<log_type::DEBUG>("window", std::format("Attempting to load text font from '{}'", jb_path));
  jb_mono_font = io.Fonts->AddFontFromFileTTF(jb_path.c_str(), 16.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
  constexpr ImWchar icon_range[] = { 0xea60, 0xec1e, 0 };
  const std::string nerd_path = asset_path<asset_type::FONT>("Nerd-Symbols");
  log<log_type::DEBUG>("window", std::format("Attempting to load icon font from '{}'", nerd_path));
  nerd_icons_font = io.Fonts->AddFontFromFileTTF(nerd_path.c_str(), 16.0f, nullptr, icon_range);
  io.Fonts->Build();

  ImGui_ImplGlfw_InitForOpenGL(win, true);
  ImGui_ImplOpenGL3_Init("#version 460");
}

window::~window() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(win);
  glfwTerminate();
}

float window::get_time() const { // NOLINT(*-convert-member-functions-to-static)
  return static_cast<float>(ImGui::GetTime());
}

float window::aspect_ratio() const {
  return io->DisplaySize.x / io->DisplaySize.y;
}

float window::delta_time_ms() const {
  return io->DeltaTime * 1000.0f;
}

float window::delta_time_s() const {
  return io->DeltaTime;
}

bool window::frame_pre() {
  glfwPollEvents();
  if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    request_close();
  }

  if (glfwGetWindowAttrib(win, GLFW_ICONIFIED) != 0) {
    ImGui_ImplGlfw_Sleep(10);
    return false;
  }

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::PushFont(jb_mono_font);
  int w, h;
  glfwGetFramebufferSize(win, &w, &h);
  glViewport(0, 0, w, h);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  return true;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void window::frame_post() {
  ImGui::PopFont();
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glfwSwapBuffers(win);
}
