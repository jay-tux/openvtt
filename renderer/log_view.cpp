//
// Created by jay on 11/29/24.
//

#include <imgui.h>

#include "log_view.hpp"
#include "window.hpp"

using namespace gltt::renderer;

namespace {
constexpr ImVec4 color_for(const log_type t) {
  switch (t) {
    case log_type::DEBUG: return {0.0f, 1.0f, 0.3f, 1.0f};
    case log_type::INFO: return {0.0f, 0.3f, 1.0f, 1.0f};
    case log_type::WARNING: return {0.8f, 0.8f, 0.0f, 1.0f};
    case log_type::ERROR: return {1.0f, 0.0f, 0.0f, 1.0f};
    default: return {0.0f, 0.0f, 0.0f, 1.0f};
  }
}
}

void log_view::render() {
  const auto &w = window::get();
  ImGui::Begin("Log Messages");

  w.with_nerd_icons([]() {
    if (ImGui::Button(reinterpret_cast<const char *>(u8""))) {
      recent_logs.clear();
      ::log<log_type::DEBUG>("logger", "Cleared!");
    }
  });
  ImGui::SameLine();
  if (ImGui::BeginChild("Scrolling")) {
    for (const auto &[src, msg, t]: recent_logs) {
      ImGui::TextColored(color_for(t), "[%10s]: %s", src.c_str(), msg.c_str());
    }
  }
  ImGui::EndChild();
  ImGui::End();
}
