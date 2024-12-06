#version 460

layout (location =  0) in vec3 pos;
layout (location =  1) in vec2 uvs;
layout (location =  2) in vec3 normal;
layout (location =  3) in mat4 model;       // 3,4,5,6  ~> 4x vec4
layout (location =  7) in mat4 model_inv_t; // 7,8,9,10 ~> 3x vec3, but padded?

layout (location =  0) uniform mat4 view;
layout (location =  1) uniform mat4 projection;

out vec2 out_uvs;
out vec3 out_normal;
out vec3 out_pos;
out vec2 out_pos_ndc;

void main() {
    gl_Position = projection * view * model * vec4(pos, 1.0);
    out_uvs = uvs;
    out_normal = normalize(mat3(model_inv_t) * normal);
    out_pos = vec3(model * vec4(pos, 1.0));
    out_pos_ndc = gl_Position.xy / gl_Position.w * 0.5 + 0.5;
}