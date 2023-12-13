#version 450


#extension GL_EXT_debug_printf : enable

#define PI 3.14159265358979323846264338327950
#define RECIPROCAL_PI 0.3183098861837907
#define printf(value) debugPrintfEXT(value)
#define assert( condition, value ) if( condition ){ \
          debugPrintfEXT( value );                    \
        }

struct VertexData {
  float x, y, z;
  float nx, ny, nz;
  float tx, ty, tz;
  float u, v;
  float _x, _y; // padding not used currently
};

struct Light {
  vec3 pos;
  vec4 color;
};

const uint MAX_NUM_OF_LIGHTS = 100;

layout (set = 0, binding = 0) uniform GlobalUbo {
  mat4 proj;
  mat4 view;
  mat4 proj_view;
  vec3 cam_pos;
  uint num_of_lights;
  Light lights[MAX_NUM_OF_LIGHTS];
  vec4 skylight_dir;
  vec4 skylight_color;
} gubo;

layout (set = 1, binding = 0) uniform ObjectUbo {
  vec4 color_factor;
  vec4 emissive_factor;
  float metallic_factor;
  float roughness_factor;
  float ao;
  float _; // padding
} oubo;

layout (set = 2, binding = 0) readonly buffer Vertices {
  VertexData data[];
} vertices;

layout (set = 2, binding = 1) readonly buffer Indices {
  uint data[];
} indices;

layout (set = 3, binding = 0) uniform sampler2D tex_albedo;
layout (set = 4, binding = 0) uniform sampler2D tex_metalic_roughness;
layout (set = 5, binding = 0) uniform sampler2D tex_normal;
layout (set = 6, binding = 0) uniform sampler2D tex_emissive;
layout (set = 7, binding = 0) uniform sampler2D env_map;

layout (push_constant) uniform constants {
  mat4 model_mat;
  // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
  mat4 norm_mat;
} push;

#ifdef VERTEX_SHADER

layout (location = 0) out vec3 o_norm;
layout (location = 1) out vec3 o_tan;
layout (location = 2) out vec3 o_bitan;
layout (location = 3) out vec3 o_pos;
layout (location = 4) out vec2 o_uv;

// vertex shader
void main() {
  //const array of positions for the triangle
  uint idx = indices.data[gl_VertexIndex];
  VertexData vtx = vertices.data[idx];
  vec4 world_space_pos = push.model_mat * vec4(vtx.x, vtx.y, vtx.z, 1.0f);
  
  
  //output the position of each vertex
  o_norm = vec3(vec4(vtx.nx, vtx.ny, vtx.nz, 1.0f) * push.norm_mat);
  o_tan =  vec3(vec4(vtx.tx, vtx.ty, vtx.tz, 1.0f) * push.model_mat);
  o_bitan = cross(o_norm, o_tan);
  // Euclidean space
  o_pos = world_space_pos.xyz / world_space_pos.w;
  o_uv = vec2(vtx.u, vtx.v);
  gl_Position = gubo.proj_view * world_space_pos;
}

#endif
#ifdef FRAGMENT_SHADER

layout (location = 0) in vec3 i_world_norm;
layout (location = 1) in vec3 i_world_tan;
layout (location = 2) in vec3 i_world_bitan;
layout (location = 3) in vec3 i_world_pos;
layout (location = 4) in vec2 i_uv;

layout (location = 0) out vec4 outFragColor;

// pbr model based on: 
// https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
// https://youtu.be/gya7x9H3mV0
// https://media.disneyanimation.com/uploads/production/publication_asset/48/asset/s2012_pbs_disney_brdf_notes_v3.pdf

// =====================================================
// specular term
vec3 fresnel_schlick(float cos_theta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
} 

float D_GGX(float NoH, float roughness) {
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;
  float NoH2 = NoH * NoH;
  float b = (NoH2 * (alpha2 - 1.0) + 1.0);
  return alpha2 / (PI * b * b);
}

float G1_GGX_Schlick(float NoV, float roughness) {
  //float r = roughness; // original
  float r = 0.5 + 0.5 * roughness; // Disney remapping
  float k = (r * r) / 2.0;
  float denom = NoV * (1.0 - k) + k;
  return max(NoV, 0.001) / denom;
}

float G_Smith(float NoV, float NoL, float roughness) {
  float g1_l = G1_GGX_Schlick(NoL, roughness);
  float g1_v = G1_GGX_Schlick(NoV, roughness);
  return g1_l * g1_v;
}
// =====================================================

vec3 microfacetBRDF(in vec3 L, in vec3 V, in vec3 N, 
              in vec3 base_color, in float metallicness, 
              in float fresnel_reflect, in float roughness) {
     
  vec3 H = normalize(V + L); // half vector

  // all required dot products
  float NoV = clamp(dot(N, V), 0.0, 1.0);
  float NoL = clamp(dot(N, L), 0.0, 1.0);
  float NoH = clamp(dot(N, H), 0.0, 1.0);
  float VoH = clamp(dot(V, H), 0.0, 1.0);     
  
  // F0 for dielectics in range [0.0, 0.16] 
  // default FO is (0.16 * 0.5^2) = 0.04
  vec3 f0 = vec3(0.16 * (fresnel_reflect * fresnel_reflect)); 
  // in case of metals, baseColor contains F0
  f0 = mix(f0, base_color, metallicness);

  // specular microfacet (cook-torrance) BRDF
  vec3 F = fresnel_schlick(VoH, f0);
  float D = D_GGX(NoH, roughness);
  float G = G_Smith(NoV, NoL, roughness);
  vec3 spec = (F * D * G) / (4.0 * max(NoV, 0.001) * max(NoL, 0.001));
  
  // diffuse
  vec3 rhoD = base_color;
  rhoD *= vec3(1.0) - F; // if not specular, use as diffuse (optional)
  rhoD *= (1.0 - metallicness); // no diffuse for metals
  vec3 diff = rhoD * RECIPROCAL_PI;
  
  return diff + spec;
}

// linear to sRGB approximation
vec3 lin_to_rgb(vec3 lin) { 
  return pow(lin, vec3(1.0 / 2.2));
}

vec2 direction_to_spherical_envmap(vec3 dir) {
  float phi = atan(dir.x, dir.z);
  float theta = acos(dir.y);
  float u = 0.5 - phi / (2.0 * PI);
  float v = 1.0 - theta / PI;
  return vec2(u, v);
}


void main() {
  // tan space
  vec3 norm = normalize(i_world_norm);
  vec3 tan = normalize(i_world_tan);
  vec3 bitan = normalize(i_world_bitan);
  mat3 tanspace_to_world = mat3(tan, bitan, norm);
  
  // unpacking normal
  vec3 N = texture(tex_normal, i_uv).rgb;
  N = normalize(N * 2.0f - 1.0f);
  N = tanspace_to_world * N;
  
  vec3 base_color = vec3(texture(tex_albedo, i_uv).rgba * oubo.color_factor);
  vec3 emission = vec3(texture(tex_emissive, i_uv).rgba * oubo.emissive_factor);

  // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_material_pbrmetallicroughness_metallicroughnesstexture
  float metallicness = texture(tex_metalic_roughness, i_uv).b * oubo.metallic_factor;
  float roughness = texture(tex_metalic_roughness, i_uv).g * oubo.roughness_factor;

  vec3 view_dir = normalize(gubo.cam_pos - i_world_pos);
  vec3 skylight_dir = normalize(gubo.skylight_dir.xyz);
  vec3 skylight_color = gubo.skylight_color.rgb;

  vec3 radiance = emission;

 // skylight
 // direct lighting
 // vec3 brdf = microfacetBRDF(skylight_dir, view_dir, N, base_color, metallicness, oubo.ao, roughness);
 // radiance += brdf * skylight_color;

 //  // sum of all point lights
 //  for(int i=0; i< gubo.num_of_lights; ++i) {
 //    Light light = gubo.lights[i];
 //    vec3 light_dir = normalize(light.pos - i_world_pos);
 //    float light_distance = length(light.pos - i_world_pos);
 //    float attenuation = 1 / (light_distance * light_distance);

 //    // irradiance contribution from light
 //    float irradiance = max(dot(light_dir, N), 0.0);
 //    if(irradiance > 0.0) {
 //      // avoid calculating brdf if light doesn't contribute
 //      vec3 brdf = microfacetBRDF(light_dir, view_dir, N, base_color, metallicness, oubo.ao, roughness);
 //      radiance += irradiance * brdf * light.color.rgb * attenuation;
 //    }
 //  }

  // IBL
  vec2 env_uv = direction_to_spherical_envmap(N);
  radiance += base_color * texture(env_map, env_uv).rgb;
  
  // printf(radiance);
  // outFragColor = vec4((N * 0.5 + 0.5), 1.0f);
  // outFragColor = vec4(base_color, 1.0f);
  // outFragColor = vec4(emission, 1.0f);
  outFragColor = vec4(lin_to_rgb(radiance), 1.0f);
}

#endif
