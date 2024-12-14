#version 460

layout(location =  0) in vec3 pos;
layout(location =  1) in mat4 model;

layout(location =  0) uniform mat4 view;
layout(location =  1) uniform mat4 projection;

out flat int self_instance;

void main() {
    gl_Position = projection * view * model * vec4(pos, 1.0);
    self_instance = gl_InstanceID;
}