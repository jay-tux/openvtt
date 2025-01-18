//
// Created by jay on 12/6/24.
//

#include <fstream>
#include <mapLexer.h>
#include <mapParser.h>

#include "map_parser.hpp"
#include "map_errors.hpp"
#include "filesys.hpp"
#include "map_visitor.hpp"
#include "renderer/render_cache.hpp"

using namespace openvtt::map;
using namespace openvtt::renderer;

map_desc map_desc::parse_from(const std::string &asset) {
  auto path = asset_path<asset_type::MAP>(asset);
  log<log_type::DEBUG>("map_parser", std::format("Loading map {}, from {}", asset, path));

  std::ifstream strm(path);
  if (!strm.is_open()) {
    log<log_type::ERROR>("map_parser", std::format("Failed to open map file {}", path));
    return {};
  }

  lexer_error_listener lex_error{path};
  parser_error_listener parse_error{path};

  antlr4::ANTLRInputStream input(strm);
  mapLexer lexer(&input);
  lexer.removeErrorListeners();
  lexer.addErrorListener(&lex_error);

  antlr4::CommonTokenStream tokens(&lexer);
  mapParser parser(&tokens);
  parser.removeErrorListeners();
  parser.addErrorListener(&parse_error);

  map_visitor visitor;
  visitor.file = path;
  visitor.visit(parser.program());

  if (visitor.highlight_binding.has_value()) {
    if (visitor.requires_highlight.empty() && visitor.requires_instanced_highlight.empty()) {
      log<log_type::WARNING>("map_parser", "Highlighting binding index provided, but no shaders require highlighting.");
    }
  }
  else if(!visitor.requires_highlight.empty() || !visitor.requires_instanced_highlight.empty()) {
    log<log_type::WARNING>("map_parser", "Highlighting binding index provided, but no shaders require highlighting.");
    visitor.requires_highlight.clear();
    visitor.requires_instanced_highlight.clear();
  }

  const auto [x, y] = visitor.map_size;
  if (x <= 0 || y <= 0) {
    log<log_type::ERROR>("map_parser", "Map size must be greater than 0.");
    return {};
  }
  const glm::vec2 offset{ -static_cast<float>(x) / 2, -static_cast<float>(y) / 2 };

  std::vector<glm::vec2> defaulted;
  std::vector<voxel_ref> voxels;
  const auto [w, h] = visitor.map_size;
  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
      if (!visitor.set_voxels.contains({i, j})) defaulted.emplace_back(glm::vec2{i, j} + offset);
    }
  }

  voxels.reserve(visitor.voxels.size() + !defaulted.empty());
  if (!defaulted.empty()) {
    if (!visitor.default_set) {
      log<log_type::WARNING>("map_parser", "Default voxels required, but no default voxel set. Did you forget to use `default {}` in your map?");
    }

    auto [back, spot, fac, alpha, beta, delta] = visitor.default_voxel;
    glm::mat4x3 tiers(0.0f);
    for (int i = 0; i < 4; i++) {
      tiers[i] = glm::vec3(alpha[i], beta[i], delta[i]);
    }

    voxels.push_back(render_cache::construct<voxel_group>(back, spot, fac, defaulted, tiers));
  }

  for (auto &[vox, pos] : visitor.voxels) {
    auto [back, spot, fac, alpha, beta, delta] = vox;
    glm::mat4x3 tiers(0.0f);
    for (int i = 0; i < 4; i++) {
      tiers[i] = glm::vec3(alpha[i], beta[i], delta[i]);
    }

    for (auto &p : pos) p += offset;
    voxels.push_back(render_cache::construct<voxel_group>(back, spot, fac, pos, tiers));
  }

  return {
    .scene = {visitor.spawned.begin(), visitor.spawned.end()},
    .scene_instances = { visitor.spawned_instances.begin(), visitor.spawned_instances.end() },
    .requires_highlight = std::move(visitor.requires_highlight),
    .requires_instanced_highlight = std::move(visitor.requires_instanced_highlight),
    .highlight_binding = visitor.highlight_binding,
    .voxels = std::move(voxels),
    .perlin_scale = visitor.perlin_scale
  };
}