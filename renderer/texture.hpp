//
// Created by jay on 11/30/24.
//

#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <string>

namespace openvtt::renderer {
/**
 * @brief A class that represents a texture.
 */
class texture {
public:
  /**
   * @brief Creates a texture from an asset.
   * @param asset The path to the asset.
   *
   * The asset path is resolved using @ref asset_path.
   */
  explicit texture(const std::string &asset);
  texture(const texture &other) = delete;
  constexpr texture(texture &&other) noexcept {
    std::swap(id, other.id);
  }
  texture &operator=(const texture &other) = delete;
  texture &operator=(texture &&other) = delete;

  /**
   * @brief Binds the texture to a slot.
   * @param slot The slot to bind the texture to.
   *
   * Practically, the `slot`-th texture unit is activated, then the texture is bound to it (using `glActivateTexture`
   * and `glBindTexture`).
   */
  void bind(unsigned int slot) const;

  ~texture();
private:
  unsigned int id = 0; ///< The OpenGL ID of the texture.
};
}

#endif //TEXTURE_HPP
