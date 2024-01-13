#version 450

#define PI 3.1415926535897932384626433832795
layout (push_constant) uniform constants {
    mat4 inv_view;
    mat4 inv_proj;
} push;

#ifdef VERTEX_SHADER
layout (location = 0) out vec2 o_uv;
layout (location = 1) out vec3 o_view_dir;

void main() {
    o_uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(o_uv * 2.0f + -1.0f, 0.0f, 1.0f);
    o_view_dir = mat3(push.inv_view) * (push.inv_proj * gl_Position).xyz;
    // o_view_dir.y =  -o_view_dir.y;
}


#endif
#ifdef FRAGMENT_SHADER
layout (location = 0) in vec2 i_uv;
layout (location = 1) in vec3 i_view_dir;
layout (location = 0) out vec4 o_color;

layout (set = 0, binding = 0) uniform sampler2D tex_input;
layout (set = 1, binding = 0) uniform sampler2D env_map;

vec2 direction_to_spherical_envmap(vec3 dir) {
  float phi = atan(dir.z, dir.x);
  float theta = acos(dir.y);
  float u = 0.5 - phi / (2.0 * PI);
  float v = 1.0 - theta / PI;
  return vec2(u, v);
}

vec3 ACESFilm(vec3 color) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return ( color *(a*color+b))/( color *(c*color+d)+e);
}

vec3 lin_to_rgb(vec3 lin) {
  return pow(lin, vec3(1.0 / 2.2));
}

void main() {
    //vec3 color = textureLod(tex_input, direction_to_spherical_envmap(i_view_dir).ts, 0).rgb;
    //color = ACESFilm(color);
    //color = lin_to_rgb(color);
    vec3 color = textureLod(tex_input, i_uv.ts, 0).rgb;
    o_color = vec4(color, 1.0f);
}
#endif
