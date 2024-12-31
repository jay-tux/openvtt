//
// Created by jay on 12/4/24.
//

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "gl_wrapper.hpp"
#include "fbo.hpp"

using namespace openvtt::renderer;

namespace {
void setup_tex(unsigned int &t, const GLenum format, const GLenum attachment, const GLenum ext_fmt, const unsigned int w, const unsigned int h) {
  GL_genTextures(1, &t);
  GL_bindTexture(GL_TEXTURE_2D, t);
  GL_texImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, ext_fmt, GL_UNSIGNED_BYTE, nullptr);
  GL_texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  GL_texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  GL_framebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, t, 0);
}
}

fbo::fbo(const unsigned int w, const unsigned int h) : w{w}, h{h} {
  GL_genFramebuffers(1, &fbo_id);
  GL_bindFramebuffer(GL_FRAMEBUFFER, fbo_id);
  setup_tex(rgb_tex, GL_RGB, GL_COLOR_ATTACHMENT0, GL_RGBA, w, h);
  log<log_type::DEBUG>("fbo", std::format("FBO {} has size {}x{}, texture {}", fbo_id, w, h, rgb_tex));
}

void fbo::bind() const {
  GL_getIntegerv(GL_VIEWPORT, vp_cache);
  GL_bindFramebuffer(GL_FRAMEBUFFER, fbo_id);
  GL_viewport(0, 0, w, h);
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void fbo::clear() const {
  bind();
  GL_clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  unbind();
}

void fbo::unbind() const {
  GL_bindFramebuffer(GL_FRAMEBUFFER, 0);
  GL_viewport(vp_cache[0], vp_cache[1], vp_cache[2], vp_cache[3]);
}

void fbo::bind_rgb_to(const unsigned int slot) const {
  // log<log_type::DEBUG>("fbo", std::format("FBO {}: binding texture {} to slot {}", fbo_id, rgb_tex, slot));
  GL_activeTexture(GL_TEXTURE0 + slot);
  GL_bindTexture(GL_TEXTURE_2D, rgb_tex);
}


bool fbo::verify() const {
  GL_bindFramebuffer(GL_FRAMEBUFFER, fbo_id);
  const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  GL_bindFramebuffer(GL_FRAMEBUFFER, 0);

#define STATUS_STR(X) case X: log<log_type::WARNING>("fbo", "Framebuffer incomplete: " #X "."); return false;
#define ERRORS \
  STATUS_STR(GL_FRAMEBUFFER_UNDEFINED) STATUS_STR(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)\
  STATUS_STR(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) STATUS_STR(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)\
  STATUS_STR(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER) STATUS_STR(GL_FRAMEBUFFER_UNSUPPORTED)\
  STATUS_STR(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE) STATUS_STR(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS)

  switch (status) {
    case GL_FRAMEBUFFER_COMPLETE:
      return true;

    ERRORS

    default:
      log<log_type::WARNING>("fbo", "Framebuffer incomplete: unknown error.");
      return false;
  }

#undef ERRORS
#undef STATUS_STR
}

void fbo::resize(const int nw, const int nh) {
  w = nw;
  h = nh;
  GL_deleteTextures(1, &rgb_tex);
  setup_tex(rgb_tex, GL_RGB, GL_COLOR_ATTACHMENT0, GL_RGBA, w, h);
}

void fbo::draw_texture_imgui(const char *name, const int w, const int h) const {
  ImGui::Begin(name);
  ImGui::Image(reinterpret_cast<ImTextureID>(reinterpret_cast<void *>(rgb_tex)), ImVec2{static_cast<float>(w), static_cast<float>(h)});
  ImGui::End();
}

fbo::~fbo() {
  GL_deleteFramebuffers(1, &fbo_id);
  GL_deleteTextures(1, &rgb_tex);
}
