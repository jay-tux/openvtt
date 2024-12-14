//
// Created by jay on 12/2/24.
//

#include "gl_wrapper.hpp"
#include "collider.hpp"
#include "filesys.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

using namespace openvtt::renderer;

namespace {
constexpr glm::vec3 element_min(const glm::vec3 &a, const glm::vec3 &b) {
  return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
}
constexpr glm::vec3 element_max(const glm::vec3 &a, const glm::vec3 &b) {
  return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
}
}

collider::collider(const std::vector<glm::vec3> &vertices, const std::vector<unsigned int> &indices)
  : vertices{vertices}, indices{indices} {
  gl(glGenVertexArrays(1, &vao));
  gl(glBindVertexArray(vao));

  min = max = vertices[0];
  std::vector<float> v_data;
  v_data.reserve(3 * vertices.size());
  for (const auto &v : vertices) {
    v_data.push_back(v.x); v_data.push_back(v.y); v_data.push_back(v.z);

    min = element_min(min, v);
    max = element_max(max, v);
  }

  gl(glGenBuffers(1, &vbo));
  gl(glBindBuffer(GL_ARRAY_BUFFER, vbo));
  gl(glBufferData(GL_ARRAY_BUFFER, v_data.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW));
  gl(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr));
  gl(glEnableVertexAttribArray(0));

  gl(glGenBuffers(1, &ebo));
  gl(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
  gl(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW));

  gl(glBindVertexArray(0));
}

collider collider::load_from(const std::string &asset) {
  Assimp::Importer importer;
  const std::string path = asset_path<asset_type::MODEL_OBJ>(asset);
  const aiScene *scene = importer.ReadFile(
    path,
    aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_GenUVCoords
  );
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    log<log_type::ERROR>("collider", std::format("Failed to load model '{}': {}", path, importer.GetErrorString()));
    return {{}, {}};
  }

  if (scene->mNumMeshes == 0) {
    log<log_type::ERROR>("collider", std::format("Model '{}' has no meshes", path));
    return {{}, {}};
  }

  if (scene->mNumMeshes != 1) {
    log<log_type::WARNING>("collider", std::format("Only single-mesh models are supported, using first mesh from '{}'", path));
  }

  const auto *mesh = scene->mMeshes[0];

  log<log_type::DEBUG>("collider", std::format("{}: {} vertices, {} faces", path, mesh->mNumVertices, mesh->mNumFaces));
  std::vector<glm::vec3> vertices;
  vertices.reserve(mesh->mNumVertices);
  for (size_t i = 0; i < mesh->mNumVertices; i++) {
    vertices.emplace_back(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
  }

  std::vector<unsigned int> indices;
  indices.reserve(mesh->mNumFaces * 3);
  for (size_t i = 0; i < mesh->mNumFaces; i++) {
    const auto &face = mesh->mFaces[i];
    if (face.mNumIndices < 3) {
      log<log_type::WARNING>("collider", std::format("Mesh '{}', face {}: skipping because it has less than 3 vertices.", path, i));
      continue;
    }
    if (face.mNumIndices > 3) {
      log<log_type::WARNING>("collider", std::format("Mesh '{}' has non-triangle faces, and triangulation failed. Using only first three vertices of face {}.", path, i));
    }

    indices.push_back(face.mIndices[0]);
    indices.push_back(face.mIndices[1]);
    indices.push_back(face.mIndices[2]);
  }

  return {vertices, indices};
}

void collider::draw() const {
  gl(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
  gl(glBindVertexArray(vao));
  gl(glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr));
  gl(glBindVertexArray(0));
  gl(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
}

float collider::ray_intersect(const ray &r, const glm::mat4 &model) const {
  // --- Check if the ray hits the (transformed) AABB ---

  const glm::vec3 transformed[8] {
    {model * glm::vec4(min.x, min.y, min.z, 1.0f)},
    {model * glm::vec4(min.x, min.y, max.z, 1.0f)},
    {model * glm::vec4(min.x, max.y, min.z, 1.0f)},
    {model * glm::vec4(min.x, max.y, max.z, 1.0f)},
    {model * glm::vec4(max.x, min.y, min.z, 1.0f)},
    {model * glm::vec4(max.x, min.y, max.z, 1.0f)},
    {model * glm::vec4(max.x, max.y, min.z, 1.0f)},
    {model * glm::vec4(max.x, max.y, max.z, 1.0f)}
  };

  glm::vec3 aabb_min = transformed[0];
  glm::vec3 aabb_max = transformed[0];

  for (int i = 1; i < 8; i++) {
    aabb_min = element_min(aabb_min, transformed[i]);
    aabb_max = element_max(aabb_max, transformed[i]);
  }

  const float tx1 = (aabb_min.x - r.point().x) * r.inv_dir().x;
  const float tx2 = (aabb_max.x - r.point().x) * r.inv_dir().x;
  const float ty1 = (aabb_min.y - r.point().y) * r.inv_dir().y;
  const float ty2 = (aabb_max.y - r.point().y) * r.inv_dir().y;
  const float tz1 = (aabb_min.z - r.point().z) * r.inv_dir().z;
  const float tz2 = (aabb_max.z - r.point().z) * r.inv_dir().z;

  const float t_min = std::max({std::min(tx1, tx2), std::min(ty1, ty2), std::min(tz1, tz2)});
  const float t_max = std::min({std::max(tx1, tx2), std::max(ty1, ty2), std::max(tz1, tz2)});

  if (t_max < t_min || t_max < 0.0f) return INFINITY; // no hit

  // --- Ray hit the AABB, now check if it hits the mesh ---
  float hit_min = INFINITY;
  const auto c = r.dir();
  for (size_t i = 0; i < indices.size(); i +=3 ) {
    const glm::vec3 p1 = model * glm::vec4(vertices[indices[i]], 1.0f);
    const glm::vec3 p2 = model * glm::vec4(vertices[indices[i+1]], 1.0f);
    const glm::vec3 p3 = model * glm::vec4(vertices[indices[i+2]], 1.0f);

    const auto a = p2 - p1;
    const auto b = p2 - p3;
    const auto d = p2 - r.point();

    const glm::mat3 A{a, b, d};
    const glm::mat3 B{a, b, c};
    const glm::mat3 A1{d, b, c};
    const glm::mat3 A2{a, d, c};

    const float alpha = determinant(B);
    const float beta = determinant(A1) / alpha;
    const float gamma = determinant(A2) / alpha;
    const float t0 = determinant(A) / alpha;

    if (beta >= 0 && gamma >= 0 && beta + gamma <= 1 && std::isfinite(t0) && t0 < hit_min) {
      hit_min = t0;
    }
  }

  return hit_min; // if no hit, still returns INFINITY, otherwise parametric distance along ray
}

void collider::bind_vao() const {
  gl(glBindVertexArray(vao));
}

collider::~collider() {
  gl(glDeleteVertexArrays(1, &vao));
  gl(glDeleteBuffers(1, &vbo));
  gl(glDeleteBuffers(1, &ebo));
}

instanced_collider::instanced_collider(collider &&coll, const std::vector<glm::mat4> &models)
  : collider(std::move(coll)), models{models} {
  bind_vao();
  gl(glGenBuffers(1, &model_vbo));
  gl(glBindBuffer(GL_ARRAY_BUFFER, model_vbo));
  int buf = 0;
  gl(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buf));
  gl(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &buf));
  gl(glBufferData(GL_ARRAY_BUFFER, models.size() * sizeof(glm::mat4), models.data(), GL_STATIC_DRAW));
  for (unsigned int i = 0; i < 4; i++) {
    // gl(glVertexAttribPointer(i + 1, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), reinterpret_cast<void *>(4 * sizeof(float))));

    gl(glVertexAttribPointer(1 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<void *>(sizeof(glm::vec4) * i)));
    gl(glEnableVertexAttribArray(1 + i));
    gl(glVertexAttribDivisor(1 + i, 1));
  }
}

void instanced_collider::draw_all() const {
  bind_vao();
  gl(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
  gl(glDrawElementsInstanced(GL_TRIANGLES, num_triangles(), GL_UNSIGNED_INT, nullptr, models.size()));
  gl(glBindVertexArray(0));
  gl(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
}

std::pair<float, size_t> instanced_collider::ray_intersect_any(const ray &r) const {
  float min_dist = INFINITY;
  size_t min_idx = -1;
  for (size_t i = 0; i < models.size(); i++) {
    if (const float dist = ray_intersect(r, models[i]); dist < min_dist) {
      min_dist = dist;
      min_idx = i;
    }
  }
  return {min_dist, min_idx};
}

instanced_collider::~instanced_collider() {
  gl(glDeleteBuffers(1, &model_vbo));
}