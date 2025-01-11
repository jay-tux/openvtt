//
// Created by jay on 12/8/24.
//

#ifndef HOVER_HIGHLIGHTER_HPP
#define HOVER_HIGHLIGHTER_HPP

#include <optional>

#include "fbo.hpp"
#include "render_cache.hpp"
#include "camera.hpp"
#include "window.hpp"

namespace openvtt::renderer {
/**
 * @brief A helper class connecting the FBO and the render cache to highlight objects under the mouse.
 */
class hover_highlighter {
public:
  /**
   * @brief Resets the highlighter.
   */
  static inline void reset() {
    force_init();
    highlight_fbo->clear();

    if (std::holds_alternative<render_ref>(last_coll)) {
      const auto &rr = std::get<render_ref>(last_coll);
      (*rr->coll)->is_hovered = false;
    }
    else if (std::holds_alternative<std::pair<instanced_render_ref, size_t>>(last_coll)) {
      const auto &[irr, _] = std::get<std::pair<instanced_render_ref, size_t>>(last_coll);
      (*irr->coll)->is_hovered = false;
    }
  }

  /**
   * @brief Checks which object is under the mouse and renders it to the FBO.
   * @param cam The camera to use for rendering and checking.
   */
  static inline void highlight_checking(const camera &cam) {
    last_coll = render_cache::mouse_over(cam);
    const auto &sh = **highlight_shader;
    highlight_fbo->bind();

    cam.set_matrices(sh, mvp[1], mvp[2]);

    if (std::holds_alternative<render_ref>(last_coll)) {
      const auto &r = *std::get<render_ref>(last_coll);
      const auto &coll = *r.coll;
      coll->is_hovered = true;

      sh.set_mat4(mvp[0], r.model());
      r.obj->draw(sh);
    }
    else if (std::holds_alternative<std::pair<instanced_render_ref, size_t>>(last_coll)) {
      const auto &[rr, inst] = std::get<std::pair<instanced_render_ref, size_t>>(last_coll);
      const auto &irr = *rr;
      const auto &coll = *irr.coll;
      coll->is_hovered = true;
      coll->highlighted_instance = inst;

      sh.set_mat4(mvp[0], coll->model(inst));
      irr.obj->draw(sh);
    }

    highlight_fbo->unbind();
  }

  /**
   * @brief Binds the FBO highlight texture to a slot.
   * @param slot The slot to bind the texture to.
   */
  static inline void bind_highlight_tex(const unsigned int slot) {
    force_init();
    highlight_fbo->bind_rgb_to(slot);
  }

  /**
   * @brief Gets the underlying FBO.
   * @return The FBO.
   */
  static inline const fbo &get_fbo() {
    force_init();
    return *highlight_fbo;
  }

private:
  static inline void force_init() {
    if (!is_init) [[unlikely]] {
      const auto size = window::get().io_data().DisplaySize;
      highlight_fbo.emplace(
        static_cast<unsigned int>(size.x), static_cast<unsigned int>(size.y)
      );
      if (!highlight_fbo->verify()) {
        log<log_type::ERROR>("hover_highlight", "Failed to create highlight FBO");
      }

      highlight_shader = render_cache::load<shader>("basic_mvp", "highlight");
      mvp[0] = (*highlight_shader)->loc_for("model");
      mvp[1] = (*highlight_shader)->loc_for("view");
      mvp[2] = (*highlight_shader)->loc_for("projection");

      is_init = true;
    }
  }

  static inline bool is_init = false;
  static inline render_cache::collision_res last_coll = render_cache::no_collision{};
  static inline std::optional<shader_ref> highlight_shader = std::nullopt;
  static inline unsigned int mvp[3]{};
  static inline std::optional<fbo> highlight_fbo = std::nullopt;
};
}

#endif //HOVER_HIGHLIGHTER_HPP
