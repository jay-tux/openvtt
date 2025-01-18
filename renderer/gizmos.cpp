//
// Created by jay on 1/18/25.
//

#include "gl_macros.hpp"
#include "gizmos.hpp"

using namespace openvtt::renderer;

void axes::draw(const camera &cam, const glm::vec3 &origin, const float length) const  {
  cam.set_matrices(s, 1, 2);
  s.set_vec3(3, origin);
  s.set_float(4, length);

  GL_disable(GL_DEPTH_TEST);
  s.set_mat4(0, x_rot);
  s.set_vec3(5, x_color);
  obj.draw(s);
  s.set_mat4(0, y_rot);
  s.set_vec3(5, y_color);
  obj.draw(s);
  s.set_mat4(0, z_rot);
  s.set_vec3(5, z_color);
  obj.draw(s);
  GL_enable(GL_DEPTH_TEST);
}
