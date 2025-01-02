//
// Created by jay on 12/6/24.
//

#ifndef MAP_PARSER_HPP
#define MAP_PARSER_HPP

#include <string>
#include <unordered_map>
#include <renderer/render_cache.hpp>

namespace openvtt::map {
struct single_highlight {
  unsigned int uniform_tex;
  unsigned int uniform_highlight;
};

struct instanced_highlight {
  unsigned int uniform_tex;
  unsigned int uniform_highlight;
  unsigned int uniform_instance_id;
};

struct map_desc {
  std::vector<renderer::render_ref> scene;
  std::vector<renderer::instanced_render_ref> scene_instances{};
  std::unordered_map<renderer::shader_ref, single_highlight> requires_highlight;
  std::unordered_map<renderer::shader_ref, instanced_highlight> requires_instanced_highlight;
  std::optional<int> highlight_binding;
  std::vector<renderer::voxel_ref> voxels;
  float perlin_scale;

  static map_desc parse_from(const std::string &asset);
};
}

#endif //MAP_PARSER_HPP
