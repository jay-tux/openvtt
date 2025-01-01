//
// Created by jay on 12/14/24.
//

#ifndef MAP_VISITOR_HPP
#define MAP_VISITOR_HPP

#include <unordered_set>
#include <mapVisitor.h>

#include "map_parser.hpp"
#include "object_cache.hpp"

namespace openvtt::map {
struct identifier {
  std::string name;
};

struct no_value {
  loc at;
};

struct map_visitor final : mapVisitor {
  std::string file;
  object_cache cache {};
  std::unordered_set<renderer::render_ref> spawned{};
  std::unordered_set<renderer::instanced_render_ref> spawned_instances{};
  std::unordered_map<renderer::shader_ref, single_highlight> requires_highlight{};
  std::unordered_map<renderer::shader_ref, instanced_highlight> requires_instanced_highlight{};
  std::optional<int> highlight_binding{};

  constexpr loc at(const antlr4::ParserRuleContext &ctx) const {
    return {ctx, file};
  }

  template <std::invocable<antlr4::ParserRuleContext *> F, typename R = std::invoke_result_t<F, antlr4::ParserRuleContext *>>
  static R with_not_null(antlr4::ParserRuleContext *c, const loc &at, F &&f) {
    if (c == nullptr) {
      renderer::log<renderer::log_type::ERROR>(
        "map_visitor",
        std::format("Unexpected null-node at {}", at.str())
      );
      return {};
    }
    return f(c);
  }

  std::any visit_through(antlr4::ParserRuleContext *c, const loc &at) {
    return with_not_null(c, at, [this](auto *ctx) { return visit(ctx); });
  }

  template <typename T>
  T visit_no_value(antlr4::ParserRuleContext *c, const loc &at, const std::string &desc) {
    const auto node = visit_through(c, at);
    if (!node.has_value()) {
      renderer::log<renderer::log_type::ERROR>("map_visitor", std::format("Expected {} at {}, but got nothing (empty node)", desc, at.str()));
      return T{};
    }

    const auto *ptr = std::any_cast<T>(&node);
    if (ptr == nullptr) {
      if constexpr(std::same_as<T, float>) {
        if (const auto *ptr2 = std::any_cast<int>(&node); ptr2 != nullptr) {
          return static_cast<float>(*ptr2);
        }
      }

      renderer::log<renderer::log_type::ERROR>("map_visitor", std::format("Expected {} at {}, but got value of type {}",
        desc, at.str(), demangle(node.type().name())
      ));
      return T{};
    }

    return *ptr;
  }

  std::optional<value> visit_maybe_value(antlr4::ParserRuleContext *c, const loc &at) {
    const auto node = visit_through(c, at);
    if (!node.has_value()) {
      return std::nullopt;
    }
    const auto *ptr = std::any_cast<value>(&node);
    if (ptr == nullptr) {
      if (const auto *ptr2 = std::any_cast<identifier>(&node); ptr2 != nullptr) {
        return cache.lookup_var(ptr2->name, at);
      }

      if (const auto *ptr2 = std::any_cast<no_value>(&node); ptr2 != nullptr) {
        renderer::log<renderer::log_type::ERROR>("map_visitor", std::format("Expected value at {}, but 'no value' generated at {}", at.str(), ptr2->at.str()));
        return std::nullopt;
      }

      renderer::log<renderer::log_type::ERROR>("map_visitor", std::format("Expected value at {}, but got something invalid ({})", at.str(), demangle(node.type().name())));
      return std::nullopt;
    }
    return *ptr;
  }

  value visit_to_value(antlr4::ParserRuleContext *c, const loc &at) {
    const auto node = visit_through(c, at);
    if (!node.has_value()) {
      renderer::log<renderer::log_type::ERROR>("map_visitor", std::format("Expected value at {}", at.str()));
      return value{};
    }
    const auto *ptr = std::any_cast<value>(&node);
    if (ptr == nullptr) {
      if (const auto *ptr2 = std::any_cast<identifier>(&node); ptr2 != nullptr) {
        return cache.lookup_var(ptr2->name, at);
      }
      renderer::log<renderer::log_type::ERROR>("map_visitor", std::format("Expected value at {}", at.str()));
      return value{};
    }
    return *ptr;
  }

  template <valid_value T>
  std::optional<T> visit_should_be(antlr4::ParserRuleContext *c, const loc &at) {
    if (const auto v = visit_to_value(c, at); v.is<T>()) return v.as<T>();
    return std::nullopt;
  }

  template <valid_value T>
  std::optional<T> visit_maybe_value(antlr4::ParserRuleContext *c, const loc &at) {
    const auto v = visit_to_value(c, at);
    if (v.is<T>()) return v.as<T>();
    if constexpr(std::same_as<T, float>) {
      if (v.is<int>()) return static_cast<float>(v.as<int>());
    }

    renderer::log<renderer::log_type::ERROR>("map_visitor",
      std::format("Expected value of type {}, but got value of type {} at {}",
        type_name<T>(), v.type_name(), this->at(*c).str()
      )
    );
    return std::nullopt;
  }

  template <valid_value T>
  T visit_expect(antlr4::ParserRuleContext *c, const loc &at) {
    return visit_maybe_value<T>(c, at) || [] { return default_value_v<T>; };
  }

  std::optional<std::vector<value>> expect_n_args(antlr4::ParserRuleContext *c, const loc &at, const size_t n) {
    auto vec = visit_no_value<std::vector<value>>(c, at, "argument (value) list");
    if (vec.size() != n) {
      renderer::log<renderer::log_type::ERROR>("map_visitor",
        std::format("Expected {} arguments, but got {} at {}", n, vec.size(), this->at(*c).str())
      );
      return std::nullopt;
    }
    return vec;
  }

  std::any visitProgram(mapParser::ProgramContext *context) override;

  std::any visitVoxelSpec(mapParser::VoxelSpecContext *context) override;

  std::any visitObjectsSpec(mapParser::ObjectsSpecContext *context) override;

  std::any visitLoadObject(mapParser::LoadObjectContext *context) override;

  std::any visitLoadInstancedObject(mapParser::LoadInstancedObjectContext *context) override;

  std::any visitLoadShader(mapParser::LoadShaderContext *context) override;

  std::any visitLoadTexture(mapParser::LoadTextureContext *context) override;

  std::any visitLoadCollider(mapParser::LoadColliderContext *context) override;

  std::any visitLoadInstancedCollider(mapParser::LoadInstancedColliderContext *context) override;

  std::any visitLoadTransform(mapParser::LoadTransformContext *context) override;

  std::any visitEmptyListExpr(mapParser::EmptyListExprContext *context) override;

  std::any visitExprList(mapParser::ExprListContext *context) override;

  std::any visitIdExpr(mapParser::IdExprContext *context) override;

  std::any visitIntExpr(mapParser::IntExprContext *context) override;

  std::any visitFloatExpr(mapParser::FloatExprContext *context) override;

  std::any visitStringExpr(mapParser::StringExprContext *context) override;

  std::any visitTupleExpr(mapParser::TupleExprContext *context) override;

  std::any visitVec3Expr(mapParser::Vec3ExprContext *context) override;

  std::any visitListExpr(mapParser::ListExprContext *context) override;

  std::any visitLoadExpr(mapParser::LoadExprContext *context) override;

  std::any visitAssignExpr(mapParser::AssignExprContext *context) override;

  std::any visitSpawnExpr(mapParser::SpawnExprContext *context) override;

  std::any visitInstancedSpawnExpr(mapParser::InstancedSpawnExprContext *context) override;

  std::any visitTransformExpr(mapParser::TransformExprContext *context) override;

  std::any visitExprStmt(mapParser::ExprStmtContext *context) override;

  std::any visitEnableHighlightStmt(mapParser::EnableHighlightStmtContext *context) override;

  std::any visitEnableInstancedHighlightStmt(mapParser::EnableInstancedHighlightStmtContext *context) override;

  std::any visitHighlightBindStmt(mapParser::HighlightBindStmtContext *context) override;

  std::any visitAddColliderStmt(mapParser::AddColliderStmtContext *context) override;

  std::any visitAddInstancedColliderStmt(mapParser::AddInstancedColliderStmtContext *context) override;

  ~map_visitor() override = default;
};
}

#endif //MAP_VISITOR_HPP
