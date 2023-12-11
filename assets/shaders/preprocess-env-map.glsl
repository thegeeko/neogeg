#version 450

#extension GL_EXT_debug_printf : enable
#define printf(msg, value) debugPrintfEXT(msg, value)
#define assert(condition, msg, value ) if(condition){ \
          debugPrintfEXT(msg, value);                    \
        }

layout (set = 0, binding = 0) uniform sampler2D env_map;
layout (set = 1, binding = 0, rgba32f) uniform writeonly image2D o_img;

layout (local_size_x = 32, local_size_y = 18, local_size_z = 1) in;

vec3 ACESFilm(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return (x*(a*x+b))/(x*(c*x+d)+e);
}

void main() {
  uvec2 i = gl_GlobalInvocationID.xy;
  vec2 uv = vec2(float(i.x) / 1280, float(i.y) / 720);

  vec3 col = ACESFilm(texture(env_map, uv).rgb);
  debugPrintfEXT("color: %v4f, pos: %v2u", col, i);
  imageStore(o_img, ivec2(i), vec4(col, 1.0f));
}
