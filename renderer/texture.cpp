//
// Created by jay on 11/30/24.
//

#include <stb_image.h>

#include "texture.hpp"
#include "gl_wrapper.hpp"
#include "filesys.hpp"

using namespace openvtt::renderer;

texture::texture(const std::string &asset) {
  log<log_type::DEBUG>("texture", std::format("Loading texture '{}'", asset));

  const std::string path = asset_path<asset_type::TEXTURE_PNG>(asset);
  int w, h, c;
  unsigned char *data = stbi_load(path.c_str(), &w, &h, &c, 4);
  if (!data) {
    log<log_type::ERROR>("texture", std::format("Failed to load texture '{}'", path));
    return;
  }

  GL_genTextures(1, &id);
  GL_bindTexture(GL_TEXTURE_2D, id);
  GL_texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  GL_texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  GL_texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  GL_texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  GL_texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  GL_generateMipmap(GL_TEXTURE_2D);
  GL_bindTexture(GL_TEXTURE_2D, 0);

  stbi_image_free(data);
}

void texture::bind(const unsigned int slot) const {
  GL_activeTexture(GL_TEXTURE0 + slot);
  GL_bindTexture(GL_TEXTURE_2D, id);
}

texture::~texture() {
  GL_deleteTextures(1, &id);
}
