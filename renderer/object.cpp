//
// Created by jay on 11/30/24.
//

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "gl_macros.hpp"
#include "window.hpp"
#include "object.hpp"
#include "filesys.hpp"

using namespace openvtt::renderer;

render_object::render_object(const std::vector<vertex_spec> &vs, const std::vector<unsigned int> &index) : elements{index.size()} {
  window::get(); // force initialized
  std::vector<float> vertex_buffer;
  vertex_buffer.reserve(vs.size() * 8);
  for (const auto &[pos, uv, norm]: vs) {
    vertex_buffer.push_back(pos.x); vertex_buffer.push_back(pos.y); vertex_buffer.push_back(pos.z);
    vertex_buffer.push_back(uv.x); vertex_buffer.push_back(uv.y);
    vertex_buffer.push_back(norm.x); vertex_buffer.push_back(norm.y); vertex_buffer.push_back(norm.z);
  }

  GL_genVertexArrays(1, &vao);
  GL_bindVertexArray(vao);

  GL_genBuffers(1, &vbo);
  GL_bindBuffer(GL_ARRAY_BUFFER, vbo);
  GL_bufferData(GL_ARRAY_BUFFER, vertex_buffer.size() * sizeof(float), vertex_buffer.data(), GL_STATIC_DRAW);
  GL_vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
  GL_enableVertexAttribArray(0);
  GL_vertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
  GL_enableVertexAttribArray(1);
  GL_vertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(5 * sizeof(float)));
  GL_enableVertexAttribArray(2);

  GL_genBuffers(1, &ebo);
  GL_bindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  GL_bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * index.size(), index.data(), GL_STATIC_DRAW);

  GL_bindVertexArray(0);
}

render_object render_object::load_from(const std::string &asset) {
  Assimp::Importer importer;
  const std::string path = asset_path<asset_type::MODEL_OBJ>(asset);
  const aiScene *scene = importer.ReadFile(
    path,
    aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_GenUVCoords
  );
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    log<log_type::ERROR>("object", std::format("Failed to load model '{}': {}", path, importer.GetErrorString()));
    return {{}, {}};
  }

  if (scene->mNumMeshes == 0) {
    log<log_type::ERROR>("object", std::format("Model '{}' has no meshes", path));
    return {{}, {}};
  }

  if (scene->mNumMeshes != 1) {
    log<log_type::WARNING>("object", std::format("Only single-mesh models are supported, using first mesh from '{}'", path));
  }

  const auto *mesh = scene->mMeshes[0];

  if (!mesh->HasNormals()) {
    log<log_type::WARNING>("object", std::format("Model '{}' has no normals, and generation failed. Using (0, 0, 0).", path));
  }

  if (!mesh->HasTextureCoords(0)) {
    log<log_type::WARNING>("object", std::format("Model '{}' has no texture coordinates, and generation failed. Using (0, 0).", path));
  }

  log<log_type::DEBUG>("object", std::format("{}: {} vertices, {} faces", path, mesh->mNumVertices, mesh->mNumFaces));
  std::vector<vertex_spec> vertices;
  vertices.reserve(mesh->mNumVertices);
  for (size_t i = 0; i < mesh->mNumVertices; i++) {
    vertex_spec spec{};
    auto &v = mesh->mVertices[i];
    spec.position = {v.x, v.y, v.z};
    if (mesh->HasNormals()) {
      spec.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
    }
    if (mesh->HasTextureCoords(0)) {
      auto &t = mesh->mTextureCoords[0][i];
      spec.uvs = {t.x, t.y};
    }
    vertices.push_back(spec);
  }

  std::vector<unsigned int> indices;
  indices.reserve(mesh->mNumFaces * 3);
  for (size_t i = 0; i < mesh->mNumFaces; i++) {
    const auto &face = mesh->mFaces[i];
    if (face.mNumIndices < 3) {
      log<log_type::WARNING>("object", std::format("Mesh '{}', face {}: skipping because it has less than 3 vertices.", path, i));
      continue;
    }
    if (face.mNumIndices > 3) {
      log<log_type::WARNING>("object", std::format("Mesh '{}' has non-triangle faces, and triangulation failed. Using only first three vertices of face {}.", path, i));
    }

    indices.push_back(face.mIndices[0]);
    indices.push_back(face.mIndices[1]);
    indices.push_back(face.mIndices[2]);
  }

  return {vertices, indices};
}

void render_object::draw(const shader &s) const {
  GL_bindVertexArray(vao);
  s.activate();
  GL_drawElements(GL_TRIANGLES, elements, GL_UNSIGNED_INT, nullptr);
}

void render_object::bind_vao() const {
  GL_bindVertexArray(vao);
}

render_object::~render_object() {
  GL_bindVertexArray(0);
  GL_deleteVertexArrays(1, &vao);
  GL_deleteBuffers(1, &vbo);
  GL_deleteBuffers(1, &ebo);
}

instanced_object::instanced_object(render_object &&ro, const std::vector<glm::mat4> &models)
  : render_object(std::move(ro)) {
  bind_vao();
  GL_genBuffers(1, &model_vbo);
  GL_bindBuffer(GL_ARRAY_BUFFER, model_vbo);
  GL_bufferData(GL_ARRAY_BUFFER, models.size() * sizeof(glm::mat4), models.data(), GL_STATIC_DRAW);
  for (unsigned int i = 0; i < 4; i++) {
    GL_enableVertexAttribArray(3 + i);
    GL_vertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void *>(sizeof(glm::vec4) * i));
    GL_vertexAttribDivisor(3 + i, 1);
  }

  auto *model_inv_t = new glm::mat4[models.size()];
  for (size_t i = 0; i < models.size(); i++) {
    model_inv_t[i] = transpose(inverse(models[i]));
  }

  GL_genBuffers(1, &model_inv_t_vbo);
  GL_bindBuffer(GL_ARRAY_BUFFER, model_inv_t_vbo);
  GL_bufferData(GL_ARRAY_BUFFER, models.size() * sizeof(glm::mat4), model_inv_t, GL_STATIC_DRAW);
  for (unsigned int i = 0; i < 4; i++) {
    GL_enableVertexAttribArray(7 + i);
    GL_vertexAttribPointer(7 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void *>(sizeof(glm::vec4) * i));
    GL_vertexAttribDivisor(7 + i, 1);
  }

  instances = models.size();

  delete [] model_inv_t;
}

void instanced_object::draw_instanced(const shader &s) const {
  bind_vao();
  s.activate();
  GL_drawElementsInstanced(GL_TRIANGLES, elements, GL_UNSIGNED_INT, nullptr, instances);
}

instanced_object::~instanced_object() {
  GL_deleteBuffers(1, &model_vbo);
  GL_deleteBuffers(1, &model_inv_t_vbo);
}

constexpr static glm::vec2 positions[9] {
  {-0.5f, -0.5f}, {0.0f, -0.5f}, {0.5f, -0.5f},
  {-0.5f,  0.0f}, {0.0f,  0.0f}, {0.5f,  0.0f},
  {-0.5f,  0.5f}, {0.0f,  0.5f}, {0.5f,  0.5f}
};

constexpr static unsigned int indices[24] {
  0, 1, 4,    0, 4, 3,
  1, 2, 5,    1, 5, 4,
  3, 4, 7,    3, 7, 6,
  4, 5, 8,    4, 8, 7
};

voxel_group::voxel_group(
  glm::vec3 background_colors[9], glm::vec3 spot_colors[9], const float factors[9],
  const std::vector<glm::vec2> &centers, const glm::mat4x3 &tiered_perlin
) : tiered_perlin{tiered_perlin}, instances{centers.size()} {
  float raw_data[81];
  for (int i = 0; i < 9; i++) {
    raw_data[9 * i + 0] = positions[i].x;
    raw_data[9 * i + 1] = positions[i].y;

    raw_data[9 * i + 2] = background_colors[i].r;
    raw_data[9 * i + 3] = background_colors[i].g;
    raw_data[9 * i + 4] = background_colors[i].b;

    raw_data[9 * i + 5] = spot_colors[i].r;
    raw_data[9 * i + 6] = spot_colors[i].g;
    raw_data[9 * i + 7] = spot_colors[i].b;

    raw_data[9 * i + 8] = factors[i];
  }

  GL_genVertexArrays(1, &vao);
  GL_bindVertexArray(vao);

  GL_genBuffers(1, &vbo);
  GL_bindBuffer(GL_ARRAY_BUFFER, vbo);
  GL_bufferData(GL_ARRAY_BUFFER, sizeof(raw_data), raw_data, GL_STATIC_DRAW);
  GL_vertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), nullptr);
  GL_enableVertexAttribArray(0);
  GL_vertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<void *>(2 * sizeof(float)));
  GL_enableVertexAttribArray(1);
  GL_vertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<void *>(5 * sizeof(float)));
  GL_enableVertexAttribArray(2);
  GL_vertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<void *>(8 * sizeof(float)));
  GL_enableVertexAttribArray(3);

  GL_genBuffers(1, &center_vbo);
  GL_bindBuffer(GL_ARRAY_BUFFER, center_vbo);
  GL_bufferData(GL_ARRAY_BUFFER, centers.size() * sizeof(glm::vec2), centers.data(), GL_STATIC_DRAW);
  GL_vertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);
  GL_vertexAttribDivisor(4, 1);
  GL_enableVertexAttribArray(4);

  GL_genBuffers(1, &ebo);
  GL_bindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  GL_bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

void voxel_group::draw(const shader &s) {
  static const shader *last = &s;
  static unsigned int uniform = s.loc_for("perlin_tiers");
  if (last != &s) {
    uniform = s.loc_for("perlin_tiers");
    last = &s;
  }

  GL_bindVertexArray(vao);
  s.activate();
  s.set_mat4x3(uniform, tiered_perlin);
  GL_drawElementsInstanced(GL_TRIANGLES, 24, GL_UNSIGNED_INT, nullptr, instances);
}

voxel_group::~voxel_group() {
  GL_deleteVertexArrays(1, &vao);
  GL_deleteBuffers(1, &vbo);
  GL_deleteBuffers(1, &center_vbo);
  GL_deleteBuffers(1, &ebo);
}