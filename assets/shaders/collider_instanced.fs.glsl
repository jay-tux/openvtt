#version 460

layout(location =  5) uniform bool highlighted;
layout(location =  6) uniform uint instance_id;

in flat int self_instance;

out vec4 frag_color;

void main() {
    if (highlighted && self_instance == instance_id) {
        frag_color = vec4(1.0, 0.7, 0.0, 1.0);
    } else {
        frag_color = vec4(0.0, 0.3, 1.0, 1.0);
    }
}