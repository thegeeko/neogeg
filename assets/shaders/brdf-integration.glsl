#version 450

#extension GL_EXT_debug_printf : enable
#define printf(msg, value) debugPrintfEXT(msg, value)
#define assert(condition, msg, value ) if(condition){ \
          debugPrintfEXT(msg, value);                    \
        }
#define PI 3.1415926535897932384626433832795

layout (set = 0, binding = 0) uniform sampler2D env_map;
layout (set = 1, binding = 0, rgba32f) uniform writeonly image2D o_brdf_map;

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

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

float G_SchlicksmithGGX(float NoL, float NoV, float roughness) {
	float k = (roughness * roughness) / 2.0;
	float GL = NoL / (NoL * (1.0 - k) + k);
	float GV = NoV / (NoV * (1.0 - k) + k);
	return GL * GV;
}

vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;
	
  float phi = 2.0 * PI * Xi.x;
  float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha2 - 1.0) * Xi.y));
  float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

  // from spherical coordinates to cartesian coordinates
  vec3 H;
  H.x = cos(phi) * sinTheta;
  H.y = sin(phi) * sinTheta;
  H.z = cosTheta;

  // from tangent-space vector to world-space sample vector
  vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
  vec3 tangent   = normalize(cross(up, N));
  vec3 bitangent = cross(N, tangent);

  vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
  return normalize(sampleVec);
} 

// from "Real Shading in Unreal Engine 4" by Brian Karis, Epic Games
vec2 integrate_brdf(vec2 p, float roughness, float NoV) {
  // view space in normal space from sphrical coords
  vec3 V;
  V.x = sqrt(1.0 - NoV * NoV); // sin theta
  V.y = 0.0;
  V.z = NoV;

  vec2 result = vec2(0.0, 0.0);
  vec3 N = vec3(0.0, 0.0, 1.0);

  uint Ns = uint(push.num_of_samples);
  for(uint n = 0u; n < Ns; n++) {
    vec3 random = rand_pcg3d(uvec3(p, n));

    vec3 H  = importanceSampleGGX(random.xy, N, roughness);
    vec3 L = reflect(-V, H);

    float VoH = clamp(dot(H, V), 0.0, 1.0);
    float NoL = clamp(L.z, 0.0, 1.0);
    float NoH = clamp(H.z, 0.0, 1.0);
    NoV = max(NoV, 1e-5); // avoid divsion by 0

    if(NoL > 0.0) {
      float G = G_SchlicksmithGGX(NoL, NoV, roughness);
      float G_Vis = G * VoH / (NoH * NoV);
      float Fc = pow(1.0 - VoH, 5.0);
      result.x += (1.0 - Fc) * G_Vis;
      result.y += Fc * G_Vis;
      if(NoV <= 0) {
        debugPrintfEXT("Fc: %f, G_Vis: %f, G:%f, NoL: %f, NoH: %f, VoH:%f", Fc, G_Vis, G, NoL, NoH, VoH);
        debugPrintfEXT("H: {x: %f, y: %f, z:%f}, L: {x: %f, y: %f, z:%f}", H.x, H.y, H.z, L.x, L.y, L.z);
      }
    }
  }

  result = result / float(Ns);
  return result;
}

void main() {
  uvec2 i = gl_GlobalInvocationID.xy;
  vec2 uv = vec2(float(i.x) / push.width, float(i.y) / push.height);
  vec2 r = integrate_brdf(i, uv.y, uv.x);
  imageStore(o_brdf_map, ivec2(i.x, i.y), vec4(r, 0.0, 1.0f));
}
