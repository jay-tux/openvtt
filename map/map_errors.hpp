//
// Created by jay on 12/31/24.
//

#ifndef MAP_ERRORS_HPP
#define MAP_ERRORS_HPP

#include <antlr4-runtime.h>
#include "renderer/log_view.hpp"

namespace openvtt::map {
/**
 * @brief A lexer error listener.
 *
 * This error listener writes all lexer errors to the log.
 */
struct lexer_error_listener final : antlr4::BaseErrorListener {
  std::string file;

  explicit lexer_error_listener(std::string file) : file(std::move(file)) {}

  inline void syntaxError(
      antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol, const size_t line,
      const size_t charPositionInLine, const std::string &msg, std::exception_ptr e
  ) override {
    renderer::log<renderer::log_type::WARNING>("map_lexer",
      std::format("{}:{}:{}: {} (on token '{}')", file, line, charPositionInLine, msg, offendingSymbol->getText())
    );
  }
};

/**
 * @brief A parser error listener.
 *
 * This error listener writes all parser errors to the log.
 */
struct parser_error_listener final : antlr4::BaseErrorListener {
  std::string file;

  explicit parser_error_listener(std::string file) : file(std::move(file)) {}

  inline void syntaxError(
      antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol, const size_t line,
      const size_t charPositionInLine, const std::string &msg, std::exception_ptr e
  ) override {
    renderer::log<renderer::log_type::WARNING>("map_parser",
      std::format("{}:{}:{}: {} (on token '{}')", file, line, charPositionInLine, msg, offendingSymbol->getText())
    );
  }
};
}

#endif //MAP_ERRORS_HPP
