#include <iostream>

#include "renderer/window.hpp"
#include "renderer/fps_counter.hpp"
#include "renderer/log_view.hpp"

using namespace gltt::renderer;

int main(int argc, const char **argv) {

  auto &win = window::get();
  log<log_type::INFO>("main", "Informational message");
  log<log_type::DEBUG>("main", "Debug message");
  log<log_type::WARNING>("main", "Warning message");
  log<log_type::ERROR>("main", "ERROR message");

  while (!win.should_close()) {
    if (!win.frame_pre()) continue;

    fps_counter::render();
    log_view::render();

    win.frame_post();
  }

  return 0;
}