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

/**
 * @brief Specialization of `std::hash` to support `std::pair<int, int>` values.
 */
template <>
struct std::hash<std::pair<int, int>> {
  std::size_t operator()(const std::pair<int, int> &p) const noexcept {
    const int both[2] { p.first, p.second };
    static_assert(sizeof(both) == sizeof(size_t));
    return std::bit_cast<size_t>(both);
  }
};

namespace openvtt::map {
/**
 * @brief A structure representing an identifier.
 */
struct identifier {
  std::string name; //!< The name of the identifier.
};

/**
 * @brief A structure representing the absence of a value.
 */
struct no_value {
  loc at; //!< The location of the absence in the map's source code.
};

/**
 * @brief A structure representing a voxel.
 *
 * A voxel has 9 customizable points: the 4 corners of a square (top-left, top-right, bottom-left, bottom-right), the
 * 4 centers of the edges (top, right, bottom, left), and the center of the square.
 * Each of these corners has a background color, a spot color, and a factor value.
 * The background and foreground corners are linked to the perlin-noise black and white values.
 * The factor value is multiplied with the perlin noise value to blend more towards background or spot color.
 * This blending should be done in GLSL using `mix(background, spot, factor * perlin_noise_value)`.
 *
 * The generated texture uses 4 octaves of perlin noise, each with its own parameters:
 * \f$\frac{\sum_{i=0}^4 \alpha_i * noise(\beta_i * coord + \delta_i)}{\sum_{i=0}^4 \alpha_i}\f$.
 * The alpha values are used to determine the influence of each octave on the final value, the beta values are used to
 * re-scale that octave, and the delta values offset the octaves from the coordinate (avoiding some artifacts).
 */
struct voxel {
  static inline std::mt19937 gen{std::random_device{}()}; //!< The random number generator.
  static inline std::uniform_real_distribution<float> dist{-1.0f, 1.0f}; //!< The random number distribution.

  glm::vec3 back[9]; //!< The background colors for the voxel's 9 corners.
  glm::vec3 spot[9]; //!< The spot colors for the voxel's 9 corners.
  float fac[9]; //!< The factor values for the voxel's 9 corners.
  float alpha[4] { 1, 0, 0, 0 }; //!< The alpha values for the perlin noise generation.
  float beta[4] { 1, 0, 0, 0 }; //!< The beta values for the perlin noise generation.
  float delta[4] { dist(gen), dist(gen), dist(gen), dist(gen) }; //!< The delta values for the perlin noise generation.
  // Tiered noise: (\Sum_i alpha_i * perlin(beta_i * x + delta_i)) / (\Sum_i alpha_i)
};

/**
 * @brief A structure to visit the parser's AST.
 *
 * The actual parser is generated by ANTLR4, and this visitor is used to traverse the AST.
 */
struct map_visitor final : mapVisitor {
  enum struct scope { NONE, VOXEL, OBJECTS };

  std::string file; //!< The file being parsed.
  std::vector<object_cache> context_stack{}; //!< The stack of variable contexts.
  std::unordered_set<renderer::render_ref> spawned{}; //!< The set of spawned renderable objects.
  std::unordered_set<renderer::instanced_render_ref> spawned_instances{}; //!< The set of spawned instanced renderable objects.
  std::unordered_map<renderer::shader_ref, single_highlight> requires_highlight{}; //!< The renderable objects that require highlighting.
  std::unordered_map<renderer::shader_ref, instanced_highlight> requires_instanced_highlight{}; //!< The instanced renderable objects that require highlighting.
  std::optional<int> highlight_binding{}; //!< The texture slot to which the highlighting FBO texture is bound.
  bool show_axes = false; //!< Whether to show the axes.
  scope current_scope = scope::NONE; //!< The current scope of the visitor.

  /**
   * @brief Searches the context stack for a variable.
   * @param name The variable name to search for.
   * @param at The location (in the source code) of the search.
   * @return The value of the variable, or an empty (void/`std::monostate`) value if the variable does not exist.
   */
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

  /**
   * @brief Creates a location from an ANTLR parser context.
   * @param ctx The ANTLR parser context.
   * @return The location of the context.
   */
  constexpr loc at(const antlr4::ParserRuleContext &ctx) const {
    return {ctx, file};
  }

  /**
   * @brief Calls the given function if the provided context is not null.
   * @tparam F The function to call (with signature `(antlr4::ParserRuleContext *) -> R`).
   * @tparam R The return type of the function (default: `std::invoke_result_t<F, antlr4::ParserRuleContext *>`).
   * @param c The context (possibly null).
   * @param at The location of parsing/visiting.
   * @param f The function to call.
   * @return The return value of the function, or an empty value if the context is null.
   *
   * If the given context is null, an empty/default value is returned (and a log message is generated).
   * Otherwise, the result of `f(c)` is returned.
   */
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

  /**
   * @brief Helper method for wrapper rules in the grammar (calls `visit` on the context if not null).
   * @param c The context to visit.
   * @param at The location of parsing/visiting.
   * @return The return value of visiting the node.
   *
   * If the given context is null, a default/empty value is returned.
   * Otherwise, the node is visited and the result is returned. This can be useful for "wrapper" rules (that have only
   * one child, and no real "functionality" of their own).
   */
  std::any visit_through(antlr4::ParserRuleContext *c, const loc &at) {
    return with_not_null(c, at, [this](auto *ctx) { return visit(ctx); });
  }

  /**
   * @brief Calls `visit` on the given context, and checks its return type.
   * @tparam T The expected return type.
   * @param c The context to visit.
   * @param at The location of parsing/visiting.
   * @param desc A description of the expected return type (for error messages).
   * @return The result of visiting the context, or a default value on errors.
   *
   * If the context is null, a default value is returned.
   * If the result of visiting the node has no value, a default value is returned (and an error message is generated,
   * stating the node is empty).
   * If the result of visiting the node is not of type `T` (using `std::any_cast`), a default value is returned (and an
   * error message is generated, stating the expected type (by description) and actual (demangled) type).
   * Otherwise, the value is returned.
   */
  template <typename T>
  T visit_type_check(antlr4::ParserRuleContext *c, const loc &at, const std::string &desc) {
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

  /**
   * @brief Calls `visit` on the given context, and checks its return type (expecting a `value`).
   * @param c The context to visit.
   * @param at The location of parsing/visiting.
   * @return Either the returned value, or an empty value on errors.
   *
   * If the context is null, `std::nullopt` is returned.
   * If the result of visiting the node has no value, `std::nullopt` is returned (and an error message is generated,
   * stating the node is empty).
   * If the result of visiting the node is an identifier, it is looked up (and dereferenced). If the identifier does not
   * exist, a default value is returned (and an error message is generated).
   * If the result of visiting the node is not a `value`, `std::nullopt` is returned (and an error message is
   * generated, stating the actual (demangled) type).
   * Otherwise, the value is returned.
   */
  std::optional<value> visit_maybe_value(antlr4::ParserRuleContext *c, const loc &at) {
    const auto node = visit_through(c, at);
    if (!node.has_value()) {
      renderer::log<renderer::log_type::ERROR>("map_visitor", std::format("Expected a value at {}, but got nothing (empty node)", at.str()));
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

  /**
   * @brief Calls `visit` on the given context, and checks its return type (expecting a `value`).
   * @param c The context to visit.
   * @param at The location of parsing/visiting.
   * @return Either the returned value, or a default value on errors.
   *
   * If the context is null, a default value is returned.
   * If the result of visiting the node has no value, a default value is returned (and an error message is generated,
   * stating the node is empty).
   * If the result of visiting the node is an identifier, it is looked up (and dereferenced). If the identifier does not
   * exist, a default value is returned (and an error message is generated).
   * If the result of visiting the node is not a `value`, a default value is returned (and an error message is
   * generated, stating the actual (demangled) type).
   * Otherwise, the value is returned.
   */
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

  /**
   * @brief Wrapper function around `visit_to_value` that checks the (contained) type of the returned value.
   * @tparam T The expected type of the value.
   * @param c The context to visit.
   * @param at The location of parsing/visiting.
   * @return Either the value (if it is of type `T`), or `std::nullopt`.
   *
   * If `visit_to_value` returns a value of type `T`, that value is returned.
   * In all other cases, `std::nullopt` is returned.
   *
   * This function provides integer-to-float promotions.
   */
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

  /**
   * @brief Wrapper around `visit_maybe_value<T>` that eliminates `std::nullopt`.
   * @tparam T The expected type of the value.
   * @param c The context to visit.
   * @param at The location of parsing/visiting.
   * @return The value (if it is of type `T`), or the default value of `T`.
   *
   * Just like `visit_maybe_value<T>`, this function provides integer-to-float promotions.
   */
  template <valid_value T>
  T visit_expect(antlr4::ParserRuleContext *c, const loc &at) {
    return visit_maybe_value<T>(c, at) || [] { return default_value_v<T>; };
  }

  /**
   * @brief Invokes the given lambda with a new stack slot.
   * @tparam F The lambda type.
   * @param f The lambda to invoke.
   *
   * This function creates a new stack slot, invokes the lambda, and then pops the stack slot.
   */
  template <std::invocable<> F>
  constexpr void with_new_context(F &&f) {
    context_stack.emplace_back();
    f();
    context_stack.pop_back();
  }

  /**
   * @brief Invokes the given lambda in a new scope.
   * @tparam s The scope to open.
   * @tparam F The lambda type.
   * @param f The lambda to invoke.
   *
   * If the current scope is not `NONE`, an error message is generated, and the lambda is not invoked.
   * If the context stack is not empty, a warning message is generated, and the context stack is cleared.
   * A new context is pushed onto the (empty) stack, giving the lambda a fresh context to work with.
   */
  template <scope s, std::invocable<> F>
  constexpr void with_scope(F &&f) {
    if (current_scope != scope::NONE) {
      renderer::log<renderer::log_type::ERROR>("map_parser", "Can't open a new scope while in another scope.");
      return;
    }

    if (!context_stack.empty()) {
      renderer::log<renderer::log_type::WARNING>("map_parser", "Entering a scope while the context stack is not empty. Clearing...");
    }

    current_scope = s;
    context_stack.clear();
    with_new_context([&f] { f(); });
    current_scope = scope::NONE;
  }

  std::any visitProgram(mapParser::ProgramContext *context) override; //!< Visitor for the `program` rule.

  std::any visitObjectsSpec(mapParser::ObjectsSpecContext *context) override; //!< Visitor for the `objectsSpec` rule.

  std::any visitExprList(mapParser::ExprListContext *context) override; //!< Visitor for the `exprList` rule.

  std::any visitIdExpr(mapParser::IdExprContext *context) override; //!< Visitor for the `idExpr` alternative.

  std::any visitTrueExpr(mapParser::TrueExprContext *context) override; //!< Visitor for the `trueExpr` alternative.

  std::any visitFalseExpr(mapParser::FalseExprContext *context) override; //!< Visitor for the `falseExpr` alternative.

  std::any visitIntExpr(mapParser::IntExprContext *context) override; //!< Visitor for the `intExpr` alternative.

  std::any visitFloatExpr(mapParser::FloatExprContext *context) override; //!< Visitor for the `floatExpr` alternative.

  std::any visitStringExpr(mapParser::StringExprContext *context) override; //!< Visitor for the `stringExpr` alternative.

  std::any visitTupleExpr(mapParser::TupleExprContext *context) override; //!< Visitor for the `tupleExpr` alternative.

  std::any visitVec3Expr(mapParser::Vec3ExprContext *context) override; //!< Visitor for the `vec3Expr` alternative.

  std::any visitEmptyListExpr(mapParser::EmptyListExprContext *context) override; //!< Visitor for the `emptyListExpr` alternative.

  std::any visitListExpr(mapParser::ListExprContext *context) override; //!< Visitor for the `listExpr` alternative.

  std::any visitParenExpr(mapParser::ParenExprContext *context) override; //!< Visitor for the `parenExpr` alternative.

  std::any visitPowExpr(mapParser::PowExprContext *context) override; //!< Visitor for the `powExpr` alternative.

  std::any visitMulDivModExpr(mapParser::MulDivModExprContext *context) override; //!< Visitor for the `mulDivModExpr` alternative.

  std::any visitAddSubExpr(mapParser::AddSubExprContext *context) override; //!< Visitor for the `addSubExpr` alternative.

  std::any visitCompExpr(mapParser::CompExprContext *context) override; //!< Visitor for the `compExpr` alternative.

  std::any visitAssignExpr(mapParser::AssignExprContext *context) override; //!< Visitor for the `assignExpr` alternative.

  std::any visitFuncExpr(mapParser::FuncExprContext *context) override; //!< Visitor for the `funcExpr` alternative.

  std::any visitExprStmt(mapParser::ExprStmtContext *context) override; //!< Visitor for the `exprStmt` alternative.

  /**
   * @brief Cleans up the map visitor.
   */
  ~map_visitor() override = default;
};
}

#endif //MAP_VISITOR_HPP
