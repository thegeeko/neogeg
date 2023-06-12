#version 450
//#extension GL_EXT_debug_printf : enable

struct VertexData {
	float x, y, z;
	float nx, ny, nz;
	float u, v;
};

layout (set = 0, binding = 0) uniform GlobalUbo {
	mat4 proj;
	mat4 view;
	mat4 proj_view;
	vec3 cam_pos;
} gubo;

layout (set = 1, binding = 0) uniform ObjectUbo {
	vec3 albedo;
	float metallic;
	float roughness;
	float ao;
} oubo;

layout (set = 2, binding = 0) readonly buffer Vertices {
	VertexData data[];
} vertices;

layout (set = 2, binding = 1) readonly buffer Indices {
	uint data[];
} indices;

layout (set = 3, binding = 0) uniform sampler2D tex_albedo;
layout (set = 4, binding = 0) uniform sampler2D tex_metalic;
layout (set = 5, binding = 0) uniform sampler2D tex_roughness;

layout (push_constant) uniform constants {
	mat4 model_mat;
	mat4 norm_mat;
} push;

#ifdef VERTEX_SHADER

layout (location = 0) out vec3 o_norm;
layout (location = 1) out vec3 o_pos;
layout (location = 2) out vec2 o_uv;

// vertex shader
void main() {
	//const array of positions for the triangle
	uint idx = indices.data[gl_VertexIndex];
	VertexData vtx = vertices.data[idx];
	vec4 world_space_pos = push.model_mat * vec4(vtx.x, vtx.y, vtx.z, 1.0f);


	//output the position of each vertex
	o_norm = normalize(mat3(push.norm_mat) * vec3(vtx.nx, vtx.ny, vtx.nz));
	o_pos = world_space_pos.xyz;
	o_uv = vec2(vtx.u, vtx.v);
	gl_Position = gubo.proj_view * world_space_pos;
}

#endif
#ifdef FRAGMENT_SHADER

layout (location = 0) in vec3 i_world_norm;
layout (location = 1) in vec3 i_world_pos;
layout (location = 2) in vec2 i_uv;

layout (location = 0) out vec4 outFragColor;

const float PI = 3.14159265359;

vec3 fresnel_schlick(float cos_theta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

float distributionGGX(vec3 N, vec3 H, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
}

float geometry_schlickGGX(float NdotV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = geometry_schlickGGX(NdotV, roughness);
	float ggx1 = geometry_schlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

void main() {
	vec3 albedo = texture(tex_albedo, i_uv).rgb;
	float metallic = texture(tex_metalic, i_uv).r;
	float roughness = texture(tex_roughness, i_uv).r;


	vec3 ws_pos = i_world_pos;
	vec3 ws_norm = normalize(i_world_norm);
	vec3 obj_dir = normalize(gubo.cam_pos - ws_pos);
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 lightPositions[4] = vec3[](vec3(0, 0, 10.0f), vec3(0, -3.0f, 15.0f), vec3(6.0f, 0, 15.0f), vec3(-3.0f, 0, 15.0f));

	vec3 Lo = vec3(0.0);
	for (int i = 0; i < 4; ++i)
	{
		vec3 L = normalize(lightPositions[i] - ws_pos);
		vec3 H = normalize(obj_dir + L);

		float distance = length(lightPositions[i] - ws_pos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = vec3(300.0f, 300.0f, 300.0f) * attenuation;

		float NDF = distributionGGX(ws_norm, H, roughness);
		float G = geometry_smith(ws_norm, ws_pos, L, roughness);
		vec3 F = fresnel_schlick(max(dot(H, ws_pos), 0.0), F0);

		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;

		vec3 numerator = NDF * G * F;
		float denominator = 4.0 * max(dot(ws_norm, ws_pos), 0.0) * max(dot(ws_norm, L), 0.0) + 0.0001;
		vec3 specular = numerator / denominator;

		// add to outgoing radiance Lo
		float NdotL = max(dot(ws_norm, L), 0.0);
		Lo += (kD * albedo / PI + specular) * radiance * NdotL;
	}

	vec3 ambient = vec3(0.03) * albedo * oubo.ao;
	vec3 color = ambient + Lo;

	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0 / 2.2));

	outFragColor = vec4(color, 1.0f);
//	outFragColor = vec4(albedo, 1.0f);
}

#endif
