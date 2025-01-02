//
// Created by jay on 12/14/24.
//

#ifndef MAP_VISITOR_HPP
#define MAP_VISITOR_HPP

#include <unordered_set>
#include <mapVisitor.h>
#include <random>

#include "map_parser.hpp"
#include "object_cache.hpp"

template <>
struct std::hash<std::pair<int, int>> {
  std::size_t operator()(const std::pair<int, int> &p) const noexcept {
    const int both[2] { p.first, p.second };
    static_assert(sizeof(both) == sizeof(size_t));
    return std::bit_cast<size_t>(both);
  }
};

namespace openvtt::map {
struct identifier {
  std::string name;
};

struct no_value {
  loc at;
};

struct voxel {
  static inline std::mt19937 gen{std::random_device{}()};
  static inline std::uniform_real_distribution<float> dist{-1.0f, 1.0f};

  glm::vec3 back[9];
  glm::vec3 spot[9];
  float fac[9];
  float alpha[4] { 1, 0, 0, 0 };
  float beta[4] { 1, 0, 0, 0 };
  float delta[4] { dist(gen), dist(gen), dist(gen), dist(gen) };
  // Tiered noise: (\Sum_i alpha_i * perlin(beta_i * x + delta_i)) / (\Sum_i alpha_i)
};

struct map_visitor final : mapVisitor {
  std::string file;
  std::vector<object_cache> context_stack{};
  std::unordered_set<renderer::render_ref> spawned{};
  std::unordered_set<renderer::instanced_render_ref> spawned_instances{};
  std::unordered_map<renderer::shader_ref, single_highlight> requires_highlight{};
  std::unordered_map<renderer::shader_ref, instanced_highlight> requires_instanced_highlight{};
  std::optional<int> highlight_binding{};
  voxel default_voxel{};
  std::pair<int, int> map_size;
  float perlin_scale;
  std::unordered_map<std::pair<int, int>, size_t> set_voxels{};
  std::vector<std::pair<voxel, std::vector<glm::vec2>>> voxels{};
  voxel voxel_in_progress{};
  bool default_set = false;

  constexpr value search_stack(const std::string &name, const loc &at) const {
    for (const auto &ctx : context_stack) {
      if (const auto v = ctx.lookup_var_maybe(name); v.has_value())
        return *v;
    }

    renderer::log<renderer::log_type::ERROR>("object_cache",
      std::format("Variable {} does not exist at {}", name, at.str())
    );
    return value{};
  }

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
        return search_stack(ptr2->name, at);
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
        return search_stack(ptr2->name, at);
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

  std::any visitEmptyListExpr(mapParser::EmptyListExprContext *context) override;

  std::any visitExprList(mapParser::ExprListContext *context) override;

  std::any visitIdExpr(mapParser::IdExprContext *context) override;

  std::any visitIntExpr(mapParser::IntExprContext *context) override;

  std::any visitFloatExpr(mapParser::FloatExprContext *context) override;

  std::any visitStringExpr(mapParser::StringExprContext *context) override;

  std::any visitTupleExpr(mapParser::TupleExprContext *context) override;

  std::any visitVec3Expr(mapParser::Vec3ExprContext *context) override;

  std::any visitListExpr(mapParser::ListExprContext *context) override;

  std::any visitAssignExpr(mapParser::AssignExprContext *context) override;

  std::any visitFuncExpr(mapParser::FuncExprContext *context) override;

  std::any visitExprStmt(mapParser::ExprStmtContext *context) override;

  std::any visitVExprStmt(mapParser::VExprStmtContext *context) override;

  std::any visitStmtBlock(mapParser::StmtBlockContext *context) override;

  std::any visitDefaultBlock(mapParser::DefaultBlockContext *context) override;

  ~map_visitor() override = default;
};
}

#endif //MAP_VISITOR_HPP
