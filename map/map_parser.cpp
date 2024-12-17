//
// Created by jay on 12/6/24.
//

#include <fstream>
#include <mapLexer.h>
#include <mapParser.h>

#include "map_parser.hpp"
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

  antlr4::ANTLRInputStream input(strm);
  mapLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  mapParser parser(&tokens);
  map_visitor visitor;
  visitor.file = path;
  visitor.visit(parser.program());

  return {
    .scene = {visitor.spawned.begin(), visitor.spawned.end()}
  };
}