//
// Created by jay on 12/6/24.
//

#include <fstream>
#include <format>
#include <antlr4-runtime/antlr4-runtime.h>
#include <mapLexer.h>
#include <mapParser.h>
#include <mapVisitor.h>

#include "map_parser.hpp"
#include "renderer/log_view.hpp"
#include "filesys.hpp"

using namespace openvtt::map;
using namespace openvtt::renderer;

namespace {
struct map_visitor final : mapVisitor {
  std::any visitProgram(mapParser::ProgramContext *context) override {
    log<log_type::INFO>("map_parser", "Parsing voxel spec...");
    auto voxels = visit(context->voxels);
    log<log_type::INFO>("map_parser", "Parsing objects spec...");
    auto objects = visit(context->objects);
    return nullptr;
  }

  std::any visitVoxelSpec(mapParser::VoxelSpecContext *context) override {
    auto w = std::stoul(context->w->getText());
    auto h = std::stoul(context->h->getText());
    log<log_type::INFO>("map_parser", std::format("Map size: {}x{}", w, h));
    return nullptr;
  }

  std::any visitObjectsSpec(mapParser::ObjectsSpecContext *context) override {
    log<log_type::INFO>("map_parser", std::format("Parsing {} statements...", context->body.size()));
    for (const auto &stmt: context->body) {
      visit(stmt);
    }
    return nullptr;
  }

  unsigned int depth = 1;

  std::string reasonable_depth_str = "| | | | | | | | | | | | | | | | | | | | | | | | | | | | | | ";
  std::any visitLoadObject(mapParser::LoadObjectContext *context) override {
    log<log_type::INFO>("map_parser", std::format("{:{}.{}}Loading object {}",
      reasonable_depth_str, 2 * depth, 2 * depth, context->asset->getText()
    ));
    return nullptr;
  }

  std::any visitLoadShader(mapParser::LoadShaderContext *context) override {
    log<log_type::INFO>("map_parser", std::format("{:{}.{}}Loading shader vs={}, fs={}",
      reasonable_depth_str, 2 * depth, 2 * depth, context->vs->getText(), context->fs->getText()
    ));
    return nullptr;
  }

  std::any visitLoadTexture(mapParser::LoadTextureContext *context) override {
    log<log_type::INFO>("map_parser", std::format("{:{}.{}}Loading texture {}",
      reasonable_depth_str, 2 * depth, 2 * depth, context->asset->getText())
    );
    return nullptr;
  }

  std::any visitLoadCollider(mapParser::LoadColliderContext *context) override {
    log<log_type::INFO>("map_parser", std::format("{:{}.{}}Loading collider {}",
      reasonable_depth_str, 2 * depth, 2 * depth, context->asset->getText())
    );
    return nullptr;
  }

  std::any visitLoadFont(mapParser::LoadFontContext *context) override {
    log<log_type::INFO>("map_parser", std::format("{:{}.{}}Loading font {}",
      reasonable_depth_str, 2 * depth, 2 * depth, context->asset->getText())
    );
    return nullptr;
  }

  std::any visitExprList(mapParser::ExprListContext *context) override {
    for (const auto &expr: context->exprs) {
      visit(expr);
    }

    return nullptr;
  }

  std::any visitIdExpr(mapParser::IdExprContext *context) override {
    log<log_type::INFO>("map_parser", std::format("{:{}.{}}Identifier: {}",
      reasonable_depth_str, 2 * depth, 2 * depth, context->x->getText()
    ));
    return nullptr;
  }

  std::any visitLoadExpr(mapParser::LoadExprContext *context) override {
    visit(context->ld);
    return nullptr;
  }

  std::any visitAssignExpr(mapParser::AssignExprContext *context) override {
    log<log_type::INFO>("map_parser", std::format("{:{}.{}}Assigning to/declaring '{}'",
      reasonable_depth_str, 2 * depth, 2 * depth, context->x->getText()
    ));
    ++depth;
    visit(context->value);
    --depth;
    return nullptr;
  }

  std::any visitCombineExpr(mapParser::CombineExprContext *context) override {
    log<log_type::INFO>("map_parser", std::format("{:{}.{}}Combining the following into a single renderable",
      reasonable_depth_str, 2 * depth, depth
    ));
    ++depth;
    visit(context->args);
    --depth;
    return nullptr;
  }

  std::any visitExprStmt(mapParser::ExprStmtContext *context) override {
    log<log_type::INFO>("map_parser", std::format("{:{}.{}}Expression statement",
      reasonable_depth_str, 2 * depth, depth
    ));
    ++depth;
    visit(context->e);
    --depth;
    return nullptr;
  }

  ~map_visitor() override = default;
};
}

void openvtt::map::parse_map(const std::string &asset) {
  auto path = asset_path<asset_type::MAP>(asset);
  log<log_type::DEBUG>("map_parser", std::format("Loading map {}, from {}", asset, path));

  std::ifstream strm(path);
  if (!strm.is_open()) {
    log<log_type::ERROR>("map_parser", std::format("Failed to open map file {}", path));
    return;
  }

  antlr4::ANTLRInputStream input(strm);
  mapLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  mapParser parser(&tokens);
  map_visitor visitor;
  visitor.visit(parser.program());
}
