//
// Created by jay on 12/6/24.
//

#ifndef MAP_PARSER_HPP
#define MAP_PARSER_HPP

#include <string>

namespace openvtt::map {
struct map_desc {
  //

  static map_desc parse_from(const std::string &asset);
};

void parse_map(const std::string &asset);
}

#endif //MAP_PARSER_HPP
