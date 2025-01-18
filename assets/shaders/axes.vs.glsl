#version 460

layout (location = 0) in vec3 pos;

layout (location = 0) uniform mat4 rot;
layout (location = 1) uniform mat4 view;
layout (location = 2) uniform mat4 proj;
layout (location = 3) uniform vec3 zero;
layout (location = 4) uniform float scale;

void main() {
    gl_Position = proj * view * (rot * vec4(pos * scale, 1.0) + vec4(zero, 0.0));
}