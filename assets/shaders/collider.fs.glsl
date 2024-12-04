#version 460

layout(location =  3) uniform bool highlighted;

out vec4 frag_color;

void main() {
    if (highlighted) {
        frag_color = vec4(1.0, 0.7, 0.0, 1.0);
    } else {
        frag_color = vec4(0.0, 0.3, 1.0, 1.0);
    }
}