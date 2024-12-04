#version 460

struct point_light {
    vec3 pos;
    vec3 diffuse;
    vec3 specular;
    vec3 attenuation;
};

layout(location =  4) uniform sampler2D tex;
layout(location =  5) uniform sampler2D spec_map;
layout(location =  6) uniform sampler2D highlight_map;

layout(location =  7) uniform vec3 view_pos;
layout(location =  8) uniform int used_point_count;
layout(location =  9) uniform float ambient_light;
layout(location = 10) uniform point_light points[10];

in vec2 out_uvs;
in vec3 out_normal;
in vec3 out_pos;

out vec4 frag_color;

vec3 apply_lighting(vec3 color, vec3 color_spec) {
    vec3 amb = ambient_light * color;

    vec3 norm = normalize(out_normal);

    vec3 diff = vec3(0, 0, 0);
    vec3 spec = vec3(0, 0, 0);
    for(int i = 0; i < used_point_count; i++) {
        float l = length(points[i].pos - out_pos);
        float attn = points[i].attenuation.x + points[i].attenuation.y * l + points[i].attenuation.z * l * l;

        vec3 light_dir = normalize(points[i].pos - out_pos);
        float fac_diff = max(dot(norm, light_dir), 0.0);
        diff += fac_diff * points[i].diffuse / attn;

        vec3 view_dir = normalize(view_pos - out_pos);
        vec3 reflectDir = reflect(-light_dir, norm);
        float fac_spec = pow(max(dot(view_dir, reflectDir), 0.0), 32);
        spec += fac_spec * points[i].specular / attn;
    }

    return amb + diff * color + spec * color_spec;
}

vec3 apply_highlight(vec3 color_in) {
    const float kernel[9] = float[9](
         1,  1,  1,
         1, -8,  1,
         1,  1,  1
    );

    vec2 screenSize = vec2(textureSize(highlight_map, 0));
    vec2 texelSize = 1.0 / screenSize;
    vec2 fc = gl_FragCoord.xy;

    float edge = 0.0;
    float mean = 0.0;

    for(int y = -1; y <= 1; y++) {
        for(int x = -1; x <= 1; x++) {
            vec2 offset = vec2(x, y) * texelSize;
            vec3 s = texture(highlight_map, (fc + offset) / screenSize).rgb;

            int kernelIndex = (y + 1) * 3 + (x + 1);
            edge += s.r * kernel[kernelIndex];
            mean += s.r / 9;
        }
    }

//    return vec3(edge, edge, edge) * 100;
    return mix(vec3(mean, mean, mean), color_in, 0.5);
}

void main() {
    vec4 color = texture(tex, out_uvs);
    if(color.a < 0.2) discard;
    vec4 color_spec = texture(spec_map, out_uvs);

    frag_color = vec4(apply_highlight(apply_lighting(color.rgb, color_spec.rgb)), color.a);
}