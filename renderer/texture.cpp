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

  gl(glGenTextures(1, &id));
  gl(glBindTexture(GL_TEXTURE_2D, id));
  gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
  gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
  gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  gl(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data));
  gl(glGenerateMipmap(GL_TEXTURE_2D));
  gl(glBindTexture(GL_TEXTURE_2D, 0));

  stbi_image_free(data);
}

void texture::bind(const unsigned int slot) const {
  gl(glActiveTexture(GL_TEXTURE0 + slot));
  gl(glBindTexture(GL_TEXTURE_2D, id));
}

texture::~texture() {
  gl(glDeleteTextures(1, &id));
}