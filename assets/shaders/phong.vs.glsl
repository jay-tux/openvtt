#version 460

struct point_light {
    vec3 pos;
    vec3 diffuse;
    vec3 specular;
    vec3 attenuation;
};

layout(location =  0) in vec3 pos;
layout(location =  1) in vec2 uvs;
layout(location =  2) in vec3 normal;

layout(location =  0) uniform mat4 model;
layout(location =  1) uniform mat4 view;
layout(location =  2) uniform mat4 projection;
layout(location =  3) uniform mat3 model_inv_t;

out vec2 out_uvs;
out vec3 out_normal;
out vec3 out_pos;

void main() {
    gl_Position = projection * view * model * vec4(pos, 1.0);
    out_uvs = uvs;
    out_normal = normalize(model_inv_t * normal);
    out_pos = vec3(model * vec4(pos, 1.0));
}