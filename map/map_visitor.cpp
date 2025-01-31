//
// Created by jay on 12/14/24.
//

#include "map_visitor.hpp"
#include "map_builtins.hpp"
#include "renderer/log_view.hpp"

using namespace openvtt::map;
using namespace openvtt::renderer;

std::any map_visitor::visitProgram(mapParser::ProgramContext *context) {
  visit_through(context->objects, at(*context));
  return no_value{at(*context)}; // no reasonable return value possible
}

std::any map_visitor::visitObjectsSpec(mapParser::ObjectsSpecContext *context) {
  with_scope<scope::OBJECTS>([this, context] {
    for (const auto stmt: context->body) {
      visit_through(stmt, at(*context));
    }
  });
  return no_value{at(*context)}; // no reasonable return value possible
}

std::any map_visitor::visitExprList(mapParser::ExprListContext *context) {
  std::vector<value> values;
  values.reserve(context->exprs.size());
  for (const auto &expr : context->exprs) {
    visit_maybe_value(expr, at(*context)) | [&values](const value &v) { values.push_back(v); };
  }
  return values;
}

std::any map_visitor::visitIdExpr(mapParser::IdExprContext *context) {
  return identifier{context->x->getText()};
}

std::any map_visitor::visitTrueExpr(mapParser::TrueExprContext *context) {
  return value{true, at(*context)};
}

std::any map_visitor::visitFalseExpr(mapParser::FalseExprContext *context) {
  return value{false, at(*context)};
}

std::any map_visitor::visitIntExpr(mapParser::IntExprContext *context) {
  return value{std::stoi(context->x->getText()), at(*context)};
}

std::any map_visitor::visitFloatExpr(mapParser::FloatExprContext *context) {
  return value{std::stof(context->x->getText()), at(*context)};
}

std::any map_visitor::visitStringExpr(mapParser::StringExprContext *context) {
  const std::string x = context->x->getText();
  return value{x.substr(1, x.size() - 2), at(*context)};
}

std::any map_visitor::visitTupleExpr(mapParser::TupleExprContext *context) {
  const auto x = visit_maybe_value(context->x, at(*context));
  const auto y = visit_maybe_value(context->y, at(*context));
  if (x.has_value() && y.has_value()) {
    return value{value_pair{*x, *y}, at(*context)};
  }
  return value{value_pair{}, at(*context)};
}

std::any map_visitor::visitVec3Expr(mapParser::Vec3ExprContext *context) {
  const auto x = visit_expect<float>(context->x, at(*context));
  const auto y = visit_expect<float>(context->y, at(*context));
  const auto z = visit_expect<float>(context->z, at(*context));
  return value{glm::vec3{x, y, z}, at(*context)};
}

std::any map_visitor::visitEmptyListExpr(mapParser::EmptyListExprContext *context) {
  return value{std::vector<value>{}, at(*context)};
}

std::any map_visitor::visitListExpr(mapParser::ListExprContext *context) {
  return value{visit_type_check<std::vector<value>>(context->exprs, at(*context), "value list"), at(*context)};
}

std::any map_visitor::visitParenExpr(mapParser::ParenExprContext *context) {
  return visit_through(context->e, at(*context));
}

std::any map_visitor::visitPowExpr(mapParser::PowExprContext *context) {
  const auto x = visit_expect<float>(context->left, at(*context));
  const auto e = visit_expect<float>(context->right, at(*context));
  return value{ std::pow(x, e), at(*context) };
}

std::any map_visitor::visitMulDivModExpr(mapParser::MulDivModExprContext *context) {
  const auto left = visit_to_value(context->left, at(*context));
  const auto right = visit_to_value(context->right, at(*context));

  if (context->op->getText() == "*") return (left * right).relocate(at(*context));
  if (context->op->getText() == "/") return (left / right).relocate(at(*context));
  if (context->op->getText() == "%") return (left % right).relocate(at(*context));

  // else
  log<log_type::ERROR>("map_parser", "Invalid operator '{}' at {}", context->op->getText(), at(*context).str());
  return no_value{at(*context)};
}

std::any map_visitor::visitAddSubExpr(mapParser::AddSubExprContext *context) {
  const auto left = visit_to_value(context->left, at(*context));
  const auto right = visit_to_value(context->right, at(*context));

  if (context->op->getText() == "+") return (left + right).relocate(at(*context));
  if (context->op->getText() == "-") return (left - right).relocate(at(*context));

  // else
  log<log_type::ERROR>("map_parser", "Invalid operator '{}' at {}", context->op->getText(), at(*context).str());
  return no_value{at(*context)};
}

std::any map_visitor::visitCompExpr(mapParser::CompExprContext *context) {
  const auto left = visit_to_value(context->left, at(*context));
  const auto right = visit_to_value(context->right, at(*context));

  if (context->op->getText() == "==") return (left == right).relocate(at(*context));
  if (context->op->getText() == "!=") return (left != right).relocate(at(*context));
  if (context->op->getText() == "<") return (left < right).relocate(at(*context));
  if (context->op->getText() == ">") return (left > right).relocate(at(*context));
  if (context->op->getText() == "<=") return (left <= right).relocate(at(*context));
  if (context->op->getText() == ">=") return (left >= right).relocate(at(*context));
  //else
  log<log_type::ERROR>("map_parser", "Invalid operator '{}' at {}", context->op->getText(), at(*context).str());
  return no_value{at(*context)};
}

std::any map_visitor::visitAssignExpr(mapParser::AssignExprContext *context) {
  return visit_maybe_value(context->value, at(*context)) | [context, this](value v) {
    const auto &var = context->x->getText();
    for (auto &ctx: context_stack | std::ranges::views::reverse) {
      if (ctx.lookup_var_maybe(var).has_value()) {
        ctx.assign(var, std::move(v), at(*context));
        return std::any{identifier{var}};
      }
    }

    // not found in stack, so declare a new var
    context_stack.back().assign(var, std::move(v), at(*context));
    return std::any{identifier{var}};
  } || [this, context]{ return no_value{at(*context)}; };
}

std::any map_visitor::visitFuncExpr(mapParser::FuncExprContext *context) {
  const auto args = visit_through(context->args, at(*context));
  if (const auto *ptr = std::any_cast<std::vector<value>>(&args); ptr) {
    return invoke_builtin(context->x->getText(), *ptr, *this, at(*context));
  }

  log<log_type::ERROR>("map_parser", std::format("Expected a list of values at {}, but got {}", at(*context).str(), demangle(args.type().name())));
  return no_value{at(*context)};
}

std::any map_visitor::visitExprStmt(mapParser::ExprStmtContext *context) {
  visit_through(context->e, at(*context));
  return no_value{at(*context)}; // no reasonable return value possible
}