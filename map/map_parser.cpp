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

  if (!visitor.requires_highlight.empty() && !visitor.highlight_binding.has_value()) {
    log<log_type::ERROR>("map_parser", "Highlighting requires a binding index, but none was provided. Ignoring all highlight bindings.");
    visitor.requires_highlight.clear();
  }
  else if (visitor.requires_highlight.empty() && visitor.highlight_binding.has_value()) {
    log<log_type::WARNING>("map_parser", "Highlighting binding index provided, but no shaders require highlighting.");
  }

  return {
    .scene = {visitor.spawned.begin(), visitor.spawned.end()},
    .scene_instances = { visitor.spawned_instances.begin(), visitor.spawned_instances.end() },
    .requires_highlight = std::move(visitor.requires_highlight),
    .highlight_binding = std::move(visitor.highlight_binding)
  };
}