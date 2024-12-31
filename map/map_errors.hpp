//
// Created by jay on 12/31/24.
//

#ifndef MAP_ERRORS_HPP
#define MAP_ERRORS_HPP

#include <antlr4-runtime.h>
#include "renderer/log_view.hpp"

namespace openvtt::map {
struct lexer_error_listener final : antlr4::BaseErrorListener {
  std::string file;

  explicit lexer_error_listener(std::string file) : file(std::move(file)) {}

  inline void syntaxError(
      antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol, const size_t line,
      const size_t charPositionInLine, const std::string &msg, std::exception_ptr e
  ) override {
    renderer::log<renderer::log_type::WARNING>("map_lexer",
      std::format("{}:{}:{}: {} (on token {})", file, line, charPositionInLine, msg, offendingSymbol->getText())
    );
    std::cerr << "Lexer error at line " << line << ":" << charPositionInLine << " " << msg << std::endl;
  }
};

struct parser_error_listener final : antlr4::BaseErrorListener {
  std::string file;

  explicit parser_error_listener(std::string file) : file(std::move(file)) {}

  inline void syntaxError(
      antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol, const size_t line,
      const size_t charPositionInLine, const std::string &msg, std::exception_ptr e
  ) override {
    renderer::log<renderer::log_type::WARNING>("map_parser",
      std::format("{}:{}:{}: {} (on token {})", file, line, charPositionInLine, msg, offendingSymbol->getText())
    );
    std::cerr << "Parser error at line " << line << ":" << charPositionInLine << " " << msg << std::endl;
  }
};
}

#endif //MAP_ERRORS_HPP
