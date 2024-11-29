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
  FONT
};

template <asset_type type>
inline std::string asset_path(const std::string &asset_name) {
  constexpr static auto type_to_dir = [](const asset_type t) {
    switch (t) {
      case asset_type::FONT: return "/fonts/";
      default: return "";
    }
  };

  constexpr static auto type_to_ext = [](const asset_type t) {
    switch (t) {
      case asset_type::FONT: return "ttf";
      default: return "";
    }
  };

  return exe_dir() + "/assets" + type_to_dir(type) + asset_name + "." + type_to_ext(type);
}
}

#endif //FILESYS_HPP
