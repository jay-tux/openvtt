#version 460

layout (location =  0) in vec2 pos; // per-vertex
layout (location =  1) in vec3 background_color; // per-vertex
layout (location =  2) in vec3 spots_color; // per-vertex
layout (location =  3) in float factor; // per-vertex
layout (location =  4) in vec2 center; // per-instance

layout (location =  0) uniform mat4 view;
layout (location =  1) uniform mat4 projection;

out vec2 perlin_coord;
out vec3 out_normal;
out vec3 out_pos;
out flat int instance_id;
out vec3 back;
out vec3 spot;
out flat float fac;

void main() {
    vec3 center_3d = vec3(center.x, 0.0, center.y);
    vec3 pos_3d = vec3(pos.x, 0.0, pos.y);

    gl_Position = projection * view * vec4(center_3d + pos_3d, 1.0);
    perlin_coord = pos + center;
    out_normal = vec3(0, 1, 0);
    out_pos = center_3d + pos_3d;
    instance_id = gl_InstanceID;

    back = background_color;
    spot = spots_color;
    fac = factor;
}