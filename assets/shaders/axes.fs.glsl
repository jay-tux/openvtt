#version 460

layout (location = 5) uniform vec3 color;

out vec4 col;

void main() {
    col = vec4(color, 1.0);
}