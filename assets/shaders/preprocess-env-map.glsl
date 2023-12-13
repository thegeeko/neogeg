#version 450

#extension GL_EXT_debug_printf : enable
#define printf(msg, value) debugPrintfEXT(msg, value)
#define assert(condition, msg, value ) if(condition){ \
          debugPrintfEXT(msg, value);                    \
        }
#define PI 3.1415926535897932384626433832795

layout (set = 0, binding = 0) uniform sampler2D env_map;
layout (set = 1, binding = 0, rgba32f) uniform writeonly image2D o_img;

layout (local_size_x = 32, local_size_y = 18, local_size_z = 1) in;


layout (push_constant) uniform constants {
    float num_of_samples;
    float mip_level;
    float width;
    float height;
} push;

// Hash Functions for GPU Rendering, Jarzynski et al.
// http://www.jcgt.org/published/0009/03/02/
vec3 rand_pcg3d(uvec3 v) {
  v = v * 1664525u + 1013904223u;
  v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
  v ^= v >> 16u;
  v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
  return vec3(v) * (1.0/float(0xffffffffu));
}

// https://youtu.be/xFsJMUS94Fs
vec3 spherical_envmap_to_direction(vec2 uv) {
  float theta = PI * (1.0 - uv.y);
  float phi = 2.0 * PI * (0.5 - uv.x);
  return vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
}

vec2 direction_to_spherical_envmap(vec3 dir) {
  float phi = atan(dir.z, dir.x);
  float theta = acos(dir.y);
  float u = 0.5 - phi / (2.0 * PI);
  float v = 1.0 - theta / PI;
  return vec2(u, v);
}

// same concept as normal mapping a matrix
// to convert from normal space to world space
mat3 get_normal_frame(in vec3 normal) {
  vec3 someVec = vec3(1.0, 0.0, 0.0);
  float dd = dot(someVec, normal);
  vec3 tangent = vec3(0.0, 1.0, 0.0);
  if(1.0 - abs(dd) > 1e-6) {
    tangent = normalize(cross(someVec, normal));
  }
  vec3 bitangent = cross(normal, tangent);
  return mat3(tangent, bitangent, normal);
}

vec3 prefilter_env_map_diffuse(in sampler2D envmap_sampler, in vec2 uv, uvec2 p) {

  vec3 normal = spherical_envmap_to_direction(uv);
  mat3 normal_transform = get_normal_frame(normal);
  vec3 result = vec3(0.0);
  uint N = uint(push.num_of_samples);

  for(uint n = 0u; n < N; n++) {
    vec3 random = rand_pcg3d(uvec3(p, n));
    float phi = 2.0 * PI * random.x;
    float theta = asin(sqrt(random.y));
    vec3 pos_local = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
    vec3 pos_world = normal_transform * pos_local;
    vec2 uv = direction_to_spherical_envmap(pos_world);
    vec3 radiance = textureLod(envmap_sampler, uv, push.mip_level).rgb;
    result += radiance;
  }

  result = result / float(N);
  return result;
}

void main() {
  uvec2 i = gl_GlobalInvocationID.xy;
  vec2 uv = vec2(float(i.x) / push.width, float(i.y) / push.height);

  vec3 col = prefilter_env_map_diffuse(env_map, uv, i);
  imageStore(o_img, ivec2(i), vec4(col, 1.0f));
}
