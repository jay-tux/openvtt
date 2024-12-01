//
// Created by jay on 11/30/24.
//

#ifndef FILESYS_HPP
#define FILESYS_HPP

#include <string>
#include <whereami.h>

namespace gltt {
inline const std::string &exe_dir() {
  static std::string dir = [] {
    const int l = wai_getExecutablePath(nullptr, 0, nullptr);
    auto *buf = new char[l + 1];
    int dir_l;
    wai_getExecutablePath(buf, l, &dir_l);
    buf[dir_l] = '\0';
    std::string res(buf);
    delete[] buf;
    return res;
  }();
  return dir;
}

enum struct asset_type {
  FONT,
  VERT_SHADER, FRAG_SHADER,
  TEXTURE_PNG,
  MODEL_OBJ
};

template <asset_type type>
inline std::string asset_path(const std::string &asset_name) {
  constexpr static auto type_to_dir = [](const asset_type t) {
    switch (t) {
      case asset_type::FONT: return "/fonts/";
      case asset_type::VERT_SHADER:
      case asset_type::FRAG_SHADER: return "/shaders/";
      case asset_type::TEXTURE_PNG: return "/textures/";
      case asset_type::MODEL_OBJ: return "/models/";
      default: return "";
    }
  };

  constexpr static auto type_to_ext = [](const asset_type t) {
    switch (t) {
      case asset_type::FONT: return "ttf";
      case asset_type::VERT_SHADER: return "vs.glsl";
      case asset_type::FRAG_SHADER: return "fs.glsl";
      case asset_type::TEXTURE_PNG: return "png";
      case asset_type::MODEL_OBJ: return "obj";
      default: return "";
    }
  };

  return exe_dir() + "/assets" + type_to_dir(type) + asset_name + "." + type_to_ext(type);
}
}

#endif //FILESYS_HPP
