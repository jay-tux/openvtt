//
// Created by jay on 11/30/24.
//

#ifndef FILESYS_HPP
#define FILESYS_HPP

#include <string>
#include <whereami.h>

/**
* @brief Main namespace for the OpenVTT project.
*/
namespace openvtt {
/**
* @brief Returns the directory of the executable.
*
* This value is only computed once, then cached for future calls.
*/
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

/**
* @brief Supported asset types.
*/
enum struct asset_type {
  FONT, //!< TTF Font (TrueType), in the /assets/fonts/ directory, using the .ttf extension.
  VERT_SHADER, //!< Vertex shader, in the /assets/shaders/ directory, using the .vs.glsl extension.
  FRAG_SHADER, //!< Fragment shader, in the /assets/shaders/ directory, using the .fs.glsl extension.
  TEXTURE_PNG, //!< PNG Texture, in the /assets/textures/ directory, using the .png extension.
  MODEL_OBJ //!< Wavefront OBJ Model, in the /assets/models/ directory, using the .obj extension.
};

/**
* @brief Returns the full, absolute path to an asset.
* @tparam type The type of asset.
* @param asset_name The name of the asset.
*
* This function will use the `type` parameter to determine the directory and extension of the asset. Only pass the name
* of the file, without the directory (unless you're using a subdirectory) or extension.
*/
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
