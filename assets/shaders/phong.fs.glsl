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
in vec2 out_pos_ndc;

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

float sobel(vec2 offsets[9], vec2 zero_pos) {
    float sobel_x[9] = float[](-1, 0, 1, -2, 0, 2, -1, 0, 1);
    float sobel_y[9] = float[](-1, -2, -1, 0, 0, 0, 1, 2, 1);

    float grad_x = 0.0;
    float grad_y = 0.0;
    for (int i = 0; i < 9; i++) {
        float tex_sample = texture(highlight_map, zero_pos + offsets[i]).r;
        grad_x += tex_sample * sobel_x[i];
        grad_y += tex_sample * sobel_y[i];
    }

    float grad = sqrt(grad_x * grad_x + grad_y * grad_y);
    return grad;
}

vec3 apply_highlight(vec3 color_in) {
    float texel_w = 1.0 / textureSize(highlight_map, 0).x; float tw = texel_w;
    float texel_h = 1.0 / textureSize(highlight_map, 0).y; float th = texel_h;

    vec2 offsets[9] = vec2[](
        vec2(-texel_w, -texel_h),   vec2(0.0, -texel_h),    vec2(texel_w, -texel_h),
        vec2(-texel_w,  0.0),       vec2(0.0,  0.0),        vec2(texel_w,  0.0),
        vec2(-texel_w,  texel_h),   vec2(0.0,  texel_h),    vec2(texel_w,  texel_h)
    );

    float sobel_grad = sobel(offsets, out_pos_ndc);
    float gof[5] = float[](-3.2307692308, -1.3846153846, 0.0, 1.3846153846, 3.2307692308);
    float gaussian[5] = float[](0.0625, 0.25, 0.375, 0.25, 0.0625);
    float blur_edge = 0.0;

    for (int i = -2; i <= 2; i++) {
        blur_edge += texture(highlight_map, out_pos_ndc + vec2(2 * gof[i + 2] * texel_w, 0.0)).r * gaussian[i + 2];
    }

    for (int i = -2; i <= 2; i++) {
        blur_edge += texture(highlight_map, out_pos_ndc + vec2(0.0, 2 * gof[i + 2] * texel_h)).r * gaussian[i + 2];
    }

    blur_edge /= 2;
    float intensity = sobel_grad * (1 - blur_edge);
    float edge = smoothstep(0.1, 0.3, intensity);

    return mix(color_in, vec3(1.0, 1.0, 0.0), edge);
}

void main() {
    vec4 color = texture(tex, out_uvs);
    if(color.a < 0.2) discard;
    vec4 color_spec = texture(spec_map, out_uvs);

    frag_color = vec4(apply_highlight(apply_lighting(color.rgb, color_spec.rgb)), color.a);
}