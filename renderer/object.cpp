//
// Created by jay on 11/30/24.
//

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "gl_wrapper.hpp"
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

  gl(glGenVertexArrays(1, &vao));
  gl(glBindVertexArray(vao));

  gl(glGenBuffers(1, &vbo));
  gl(glBindBuffer(GL_ARRAY_BUFFER, vbo));
  gl(glBufferData(GL_ARRAY_BUFFER, vertex_buffer.size() * sizeof(float), vertex_buffer.data(), GL_STATIC_DRAW));
  gl(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr));
  gl(glEnableVertexAttribArray(0));
  gl(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float))));
  gl(glEnableVertexAttribArray(1));
  gl(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(5 * sizeof(float))));
  gl(glEnableVertexAttribArray(2));

  gl(glGenBuffers(1, &ebo));
  gl(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
  gl(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * index.size(), index.data(), GL_STATIC_DRAW));

  gl(glBindVertexArray(0));
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
  gl(glBindVertexArray(vao));
  s.activate();
  gl(glDrawElements(GL_TRIANGLES, elements, GL_UNSIGNED_INT, nullptr));
}

void render_object::bind_vao() const {
  gl(glBindVertexArray(vao));
}

render_object::~render_object() {
  gl(glBindVertexArray(0));
  gl(glDeleteVertexArrays(1, &vao));
  gl(glDeleteBuffers(1, &vbo));
  gl(glDeleteBuffers(1, &ebo));
}

instanced_object::instanced_object(render_object &&ro, const std::vector<glm::mat4> &models)
  : render_object(std::move(ro)) {
  bind_vao();
  gl(glGenBuffers(1, &model_vbo));
  gl(glBindBuffer(GL_ARRAY_BUFFER, model_vbo));
  gl(glBufferData(GL_ARRAY_BUFFER, models.size() * sizeof(glm::mat4), models.data(), GL_STATIC_DRAW));
  for (unsigned int i = 0; i < 4; i++) {
    gl(glEnableVertexAttribArray(3 + i));
    gl(glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void *>(sizeof(glm::vec4) * i)));
    gl(glVertexAttribDivisor(3 + i, 1));
  }

  auto *model_inv_t = new glm::mat4[models.size()];
  for (size_t i = 0; i < models.size(); i++) {
    model_inv_t[i] = transpose(inverse(models[i]));
  }

  gl(glGenBuffers(1, &model_inv_t_vbo));
  gl(glBindBuffer(GL_ARRAY_BUFFER, model_inv_t_vbo));
  gl(glBufferData(GL_ARRAY_BUFFER, models.size() * sizeof(glm::mat4), model_inv_t, GL_STATIC_DRAW));
  for (unsigned int i = 0; i < 3; i++) {
    gl(glEnableVertexAttribArray(7 + i));
    gl(glVertexAttribPointer(7 + i, 3, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void *>(sizeof(glm::vec4) * i)));
    gl(glVertexAttribDivisor(7 + i, 1));
  }

  instances = models.size();

  delete [] model_inv_t;
}

void instanced_object::draw_instanced(const shader &s) const {
  bind_vao();
  s.activate();
  gl(glDrawElementsInstanced(GL_TRIANGLES, elements, GL_UNSIGNED_INT, nullptr, instances));
}

instanced_object::~instanced_object() {
  gl(glDeleteBuffers(1, &model_vbo));
  gl(glDeleteBuffers(1, &model_inv_t_vbo));
}
