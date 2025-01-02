#version 460

struct point_light {
    vec3 pos;
    vec3 diffuse;
    vec3 specular;
    vec3 attenuation;
};

layout (location =  2) uniform float perlin_scale;
layout (location =  3) uniform mat4x3 perlin_tiers;
layout (location =  4) uniform vec3 view_pos;
layout (location =  5) uniform int used_point_count;
layout (location =  6) uniform float ambient_light;
layout (location =  7) uniform point_light points[10];

in vec2 perlin_coord;
in vec3 out_normal;
in vec3 out_pos;
in flat int instance_id;
in vec3 back;
in vec3 spot;
in flat float fac;

out vec4 frag_color;

vec3 mod289(vec3 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}
vec4 mod289(vec4 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x) {
    return mod289(((x * 34.0) + 1.0) * x);
}

vec3 fade(vec3 t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

float perlin(vec3 xyz) {
    vec3 i = floor(xyz);
    vec3 i1 = i + 1.0;
    i = mod289(i);
    i1 = mod289(i1);

    vec3 f = fract(xyz);
    vec3 f1 = f - 1.0;

    vec4 ix = vec4(i.x, i1.x, i.x, i1.x);
    vec4 iy = vec4(i.yy, i1.yy);
    vec4 iz0 = i.zzzz;
    vec4 iz1 = i.zzzz + 1.0;

    vec4 ixy = permute(permute(ix) + iy);
    vec4 ixy0 = permute(ixy + iz0);
    vec4 ixy1 = permute(ixy + iz1);

    vec4 gx0 = ixy0 * (1.0 / 7.0);
    vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
    gx0 = fract(gx0);
    vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
    vec4 sz0 = step(gz0, vec4(0.0));
    gx0 -= sz0 * (step(0.0, gx0) - 0.5);
    gy0 -= sz0 * (step(0.0, gy0) - 0.5);

    vec4 gx1 = ixy1 * (1.0 / 7.0);
    vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
    gx1 = fract(gx1);
    vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
    vec4 sz1 = step(gz1, vec4(0.0));
    gx1 -= sz1 * (step(0.0, gx1) - 0.5);
    gy1 -= sz1 * (step(0.0, gy1) - 0.5);

    vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
    vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
    vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
    vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
    vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
    vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
    vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
    vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

    vec4 norm0 = sqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
    g000 *= norm0.x;
    g010 *= norm0.y;
    g100 *= norm0.z;
    g110 *= norm0.w;
    vec4 norm1 = sqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
    g001 *= norm1.x;
    g011 *= norm1.y;
    g101 *= norm1.z;
    g111 *= norm1.w;

    float n000 = dot(g000, f);
    float n100 = dot(g100, vec3(f1.x, f.yz));
    float n010 = dot(g010, vec3(f.x, f1.y, f.z));
    float n110 = dot(g110, vec3(f1.xy, f.z));
    float n001 = dot(g001, vec3(f.xy, f1.z));
    float n101 = dot(g101, vec3(f1.x, f.y, f1.z));
    float n011 = dot(g011, vec3(f.x, f1.yz));
    float n111 = dot(g111, f1);

    vec3 fade_xyz = fade(f);
    vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
    vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
    float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x);
    return clamp(2.2 * n_xyz, 0.0, 1.0);
}

vec4 perlin_color() {
    float perlin_fac = 0;
    float div = 0;
    for(int i = 0; i < 4; i++) {
        vec3 local_coord = vec3(perlin_tiers[i][1] * perlin_coord * perlin_scale + perlin_tiers[i][2], 1.0);
        div += perlin_tiers[i][0];
        perlin_fac += perlin_tiers[i][0] * perlin(local_coord);
    }
    perlin_fac /= div;

//    perlin_fac = perlin(vec3(perlin_coord, 1.0));
    return vec4(mix(back / 256, spot / 256, perlin_fac * fac), 1.0);
}

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

void main() {
    vec4 color = perlin_color();
    frag_color = vec4(apply_lighting(color.rgb, color.rgb), color.a);
}