//
// Created by jay on 12/6/24.
//

#ifndef MAP_PARSER_HPP
#define MAP_PARSER_HPP

#include <string>
#include <unordered_map>
#include <renderer/render_cache.hpp>

namespace openvtt::map {
/**
 * @brief A helper descriptor type for objects that need highlighting support.
 */
struct single_highlight {
  unsigned int uniform_tex; //!< The uniform for the highlighting FBO texture.
  unsigned int uniform_highlight; //!< The uniform for determining if the object is highlighted.
};

/**
 * @brief A helper descriptor type for instanced objects that need highlighting support.
 */
struct instanced_highlight {
  unsigned int uniform_tex; //!< The uniform for the highlighting FBO texture.
  unsigned int uniform_highlight; //!< The uniform for determining if the object is highlighted.
  unsigned int uniform_instance_id; //!< The uniform containing the highlighted instance ID.
};

/**
 * @brief Structure representing the description of a loaded map.
 */
struct map_desc {
  std::vector<renderer::render_ref> scene; //!< All the renderable objects in the scene.
  std::vector<renderer::instanced_render_ref> scene_instances{}; //!< All the instanced renderable objects in the scene.
  std::unordered_map<renderer::shader_ref, single_highlight> requires_highlight; //!< The renderable objects that require highlighting.
  std::unordered_map<renderer::shader_ref, instanced_highlight> requires_instanced_highlight; //!< The instanced renderable objects that require highlighting.
  std::optional<int> highlight_binding; //!< The texture slot to which the highlighting FBO texture is bound.
  bool show_axes; //!< Whether to show the axes' gizmo.

  /**
   * @brief Parses a map from an asset file.
   * @param asset The map file to parse.
   * @return The parsed map description.
   *
   * The returned map is a best-effort one.
   * If parsing fails, a partial map might be returned.
   */
  static map_desc parse_from(const std::string &asset);
};
}

#endif //MAP_PARSER_HPP
