//
// Created by jay on 12/6/24.
//

#ifndef MAP_PARSER_HPP
#define MAP_PARSER_HPP

#include <string>
#include <unordered_map>
#include <renderer/render_cache.hpp>

namespace openvtt::map {
struct font { std::string asset; };
struct map_desc {
  std::vector<renderer::render_ref> scene;
  std::vector<renderer::shader_ref> requires_highlight;

  static map_desc parse_from(const std::string &asset);
};
}

#endif //MAP_PARSER_HPP
